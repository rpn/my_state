// my_sml_2.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace my_sml_2 {
namespace sml = boost::sml;

enum class ContextState {
	IDLE,
	WALK,
	RUN,
	JUMP,
};


struct OnInput {
    float delta_time;
    float input_power;
    bool jump = false;
};

struct Context {
	float speed {};
	ContextState state {};
};

auto is_idle    = [](const OnInput& e) { return e.input_power < 0.1f; };
auto is_walking = [](const OnInput& e) { return e.input_power >= 0.1f && e.input_power < 0.8f; };
auto is_running = [](const OnInput& e) { return e.input_power >= 0.8f; };
auto is_jumping = [](const OnInput& e) { return e.jump == true; };

struct GroundSm {
    auto operator()() const {
        using namespace sml;

        auto to_idle = [](Context& pl) { pl.state = ContextState::IDLE; };
        auto to_walk = [](Context& pl) { pl.state = ContextState::WALK; };
        auto to_run = [](Context& pl) { pl.state = ContextState::RUN; };
        
        return make_transition_table(
	        *state<class IDLE> + on_entry<_> / to_idle,
	        state<class WALK> + on_entry<_> / to_walk,
	        state<class RUN> + on_entry<_> / to_run,

	        *state<class IDLE> + event<OnInput> [is_walking] = state<class WALK>,
	        state<class IDLE> + event<OnInput> [is_running] = state<class RUN>,
	        state<class WALK> + event<OnInput> [is_running] = state<class RUN>,
	        state<class WALK> + event<OnInput> [is_idle] = state<class IDLE>,
	        state<class RUN> + event<OnInput> [is_walking] = state<class WALK>,
	        state<class RUN> + event<OnInput> [is_idle] = state<class IDLE>
        );
    }
};

struct MovementSm {
    auto operator()() const {
        using namespace sml;

		auto to_jump = [](Context& pl) { pl.state = ContextState::JUMP; };

        return make_transition_table(
	        *state<GroundSm> + event<OnInput> [is_jumping] / to_jump = state<class JUMP>,
	        state<class JUMP> + event<OnInput> [is_idle] = state<GroundSm>,
	        state<class JUMP> + event<OnInput> [is_walking] = state<GroundSm>,
	        state<class JUMP> + event<OnInput> [is_running] = state<GroundSm>
        );
    }
};

struct Player: Context {
	sml::sm<MovementSm> sm;
	Player()
		: sm(static_cast<Context&>(*this))
	{
	}

	void update(float delta_time, float input_power, bool jump)
	{
		speed = input_power;
		sm.process_event(OnInput{delta_time, input_power, jump});
	}
};

TEST(MySml2, test1)
{
	Context ctx;
    ASSERT_EQ(ContextState::IDLE, ctx.state);
    ASSERT_EQ(0.0f, ctx.speed);

    Player pl;
    ASSERT_EQ(ContextState::IDLE, pl.state);
    //ASSERT_EQ(0.0f, ctx.speed);

    pl.update(1.0f, 0.1f, false);
    ASSERT_EQ(ContextState::WALK, pl.state);

    pl.update(1.0f, 0.1f, true);
    ASSERT_EQ(ContextState::JUMP, pl.state);

    pl.update(1.0f, 0.1f, false);
    ASSERT_EQ(ContextState::WALK, pl.state);
}

} // namespace my_sml_2
