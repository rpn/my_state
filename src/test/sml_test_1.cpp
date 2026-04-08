// sml_test_1.cpp
#include <gtest/gtest.h>

#if __has_include(<boost/sml.hpp>)
#include <boost/sml.hpp>

namespace sml_test_1 {
namespace sml = boost::sml;

struct EnemySpotted {};
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

        auto toCombat = [](Context& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
        };

        return make_transition_table(
            *state<class Patrol> + event<EnemySpotted> / toCombat = state<CombatSm>,
             state<CombatSm> + event<EnemyLost> / toPatrol = state<class Patrol>
        );
    }
};

TEST(SmlTest1, inherited_style_with_sub_state)
{
    Context ctx;
    sml::sm<RootSm> sm{ ctx };

    ASSERT_EQ(TopState::Patrol, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);

    sm.process_event(EnemySpotted{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Chase, ctx.sub);

    sm.process_event(InRange{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Attack, ctx.sub);

    sm.process_event(OutOfRange{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Chase, ctx.sub);

    sm.process_event(EnemyLost{});
    ASSERT_EQ(TopState::Patrol, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);
}

} // namespace sml_test_1

#else

TEST(SmlTest1, dependency_missing)
{
    GTEST_SKIP() << "boost/sml.hpp not found. Add Boost.Ext SML include path to enable this sample.";
}

#endif
