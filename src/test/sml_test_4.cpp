// sml_test_4.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>
#include <functional>
#include <memory>
#include <random>

namespace sml_test_4 {
namespace sml = boost::sml;

struct EnemySpotted {
	int target = 0;
};
struct EnemyLost {};
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
    TopState top = TopState::Patrol;
    CombatSubState sub = CombatSubState::None;
    std::function<bool()> spotted_transition_decider = [] {
        static thread_local std::mt19937 rng{std::random_device{}()};
        static thread_local std::bernoulli_distribution coin_flip{0.5};
        return coin_flip(rng);
    };
};

struct CombatSm {
    auto operator()() const {
        using namespace sml;

        auto toChase = [](Context& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
        };

        auto toAttack = [](Context& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Attack;
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
        };

        auto toCombat = [](const EnemySpotted& e, Context& ctx) {
            ctx.target = e.target;
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
        };

        auto canEnterCombat = [](Context& ctx) {
            return ctx.spotted_transition_decider();
        };

        return make_transition_table(
            // 初期状態は Patrol
            // Patrol 状態で EnemySpotted を受け、canEnterCombat が true のときだけ toCombat を実行して CombatSm へ遷移
            *state<class Patrol> + event<EnemySpotted> [canEnterCombat] / toCombat = state<CombatSm>,

            // CombatSm 状態で EnemyLost を受けたら、toPatrol を実行して Patrol へ戻る
             state<CombatSm> + event<EnemyLost> / toPatrol = state<class Patrol>
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
}

} // namespace sml_test_4
