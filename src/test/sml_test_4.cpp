// sml_test_4.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>
#include <memory>

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
            *state<class Chase> + event<InRange> / toAttack = state<class Attack>,
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

        return make_transition_table(
            *state<class Patrol> + event<EnemySpotted> / toCombat = state<CombatSm>,
             state<CombatSm> + event<EnemyLost> / toPatrol = state<class Patrol>
        );
    }
};


TEST(SmlTest4, test1)
{
    Context ctx;
    sml::sm<RootSm> sm{ ctx };

    sm.process_event(EnemySpotted{123});
    ASSERT_EQ(123, ctx.target);
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Chase, ctx.sub);
}

} // namespace sml_test_4
