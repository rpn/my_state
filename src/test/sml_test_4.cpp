// sml_test_4.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace sml_test_4 {
namespace sml = boost::sml;

struct EnemySpotted {
	int target = 0;
};
struct EnemyLost {};
struct on_update {
    float delta_time = 0.0f;
};
struct InRange {};
struct OutOfRange {};

enum class TopState {
    Patrol,
    Combat,
};

enum class CombatSubState {
    None,
    Chase,
    Attack,
};


struct Context {
	int target = 0;
    float lost_wait_elapsed = 0.0f;
    TopState top = TopState::Patrol;
    CombatSubState sub = CombatSubState::None;
    std::function<bool()> spotted_transition_decider = [] {
        static thread_local std::mt19937 rng{std::random_device{}()};
        static thread_local std::bernoulli_distribution coin_flip{0.5};
        return coin_flip(rng);
    };

    std::vector<std::string> logger;
};

struct CombatSm {
    auto operator()() const {
        using namespace sml;

        auto toChase = [](Context& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
            ctx.logger.push_back("transition: Attack -> Chase");
        };

        auto toAttack = [](Context& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Attack;
            ctx.logger.push_back("transition: Chase -> Attack");
        };

        return make_transition_table(
            // 初期状態は Chase
            // Chase 状態で InRange イベントが来たら、toAttack を実行して Attack へ遷移
            *state<class Chase> + event<InRange> / toAttack = state<class Attack>,

            // Attack 状態で OutOfRange イベントが来たら、toChase を実行して Chase へ遷移
             state<class Attack> + event<OutOfRange> / toChase = state<class Chase>
        );
    }
};

struct RootSm {
    auto operator()() const {
        using namespace sml;

        auto toPatrol = [](Context& ctx) {
            ctx.top = TopState::Patrol;
            ctx.sub = CombatSubState::None;
            ctx.lost_wait_elapsed = 0.0f;
            ctx.target = 0;
            ctx.logger.push_back("transition: LostAfterEnemyLost -> Patrol");
        };

        auto toCombat = [](const EnemySpotted& e, Context& ctx) {
            ctx.target = e.target;
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
            ctx.logger.push_back("transition: Patrol -> CombatSm(Chase)");
        };

        auto canEnterCombat = [](Context& ctx) {
            return ctx.spotted_transition_decider();
        };

        auto beginLostWait = [](Context& ctx) {
            ctx.lost_wait_elapsed = 0.0f;
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::None;
            ctx.logger.push_back("transition: CombatSm -> LostAfterEnemyLost");
        };

        auto tickLostWait = [](const on_update& e, Context& ctx) {
            ctx.lost_wait_elapsed += e.delta_time;
        };

        auto isLostWaitDone = [](const on_update& e, const Context& ctx) {
            return (ctx.lost_wait_elapsed + e.delta_time) >= 1.0f;
        };

        return make_transition_table(
            // 初期状態は Patrol
            // Patrol 状態で EnemySpotted を受け、canEnterCombat が true のときだけ toCombat を実行して CombatSm へ遷移
            *state<class Patrol> + event<EnemySpotted> [canEnterCombat] / toCombat = state<CombatSm>,

            // CombatSm 状態で EnemyLost を受けたら、待機状態へ入る
             state<CombatSm> + event<EnemyLost> / beginLostWait = state<class LostAfterEnemyLost>,

            // 待機中は on_update で経過時間を積算し、1秒到達で Patrol へ戻る
             state<class LostAfterEnemyLost> + event<on_update> [isLostWaitDone] / toPatrol = state<class Patrol>,
             state<class LostAfterEnemyLost> + event<on_update> / tickLostWait
        );
    }
};


TEST(SmlTest4, test1)
{
    Context ctx;
    ctx.spotted_transition_decider = [] { return true; };
    sml::sm<RootSm> sm{ ctx };

    sm.process_event(EnemySpotted{123});
    ASSERT_EQ(123, ctx.target);
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Chase, ctx.sub);
    ASSERT_EQ(1u, ctx.logger.size());
    ASSERT_EQ("transition: Patrol -> CombatSm(Chase)", ctx.logger[0]);
}

TEST(SmlTest4, spotted_but_not_transitioned)
{
    Context ctx;
    ctx.spotted_transition_decider = [] { return false; };
    sml::sm<RootSm> sm{ ctx };

    sm.process_event(EnemySpotted{999});
    ASSERT_EQ(0, ctx.target);
    ASSERT_EQ(TopState::Patrol, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);
    ASSERT_TRUE(ctx.logger.empty());
}

TEST(SmlTest4, enemy_lost_waits_1s_then_returns_patrol)
{
    Context ctx;
    ctx.spotted_transition_decider = [] { return true; };
    sml::sm<RootSm> sm{ ctx };

    sm.process_event(EnemySpotted{123});
    ASSERT_EQ(TopState::Combat, ctx.top);

    sm.process_event(EnemyLost{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);
    ASSERT_EQ(0.0f, ctx.lost_wait_elapsed);

    sm.process_event(on_update{0.4f});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(0.4f, ctx.lost_wait_elapsed);

    sm.process_event(on_update{0.6f});
    ASSERT_EQ(TopState::Patrol, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);
    ASSERT_EQ(0.0f, ctx.lost_wait_elapsed);
    ASSERT_EQ(0, ctx.target);

    ASSERT_EQ(3u, ctx.logger.size());
    ASSERT_EQ("transition: Patrol -> CombatSm(Chase)", ctx.logger[0]);
    ASSERT_EQ("transition: CombatSm -> LostAfterEnemyLost", ctx.logger[1]);
    ASSERT_EQ("transition: LostAfterEnemyLost -> Patrol", ctx.logger[2]);
}

} // namespace sml_test_4
