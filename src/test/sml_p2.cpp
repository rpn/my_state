// sml_p2.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_p2 {
namespace sml = boost::sml;


namespace logic {

bool is_idle(float input_power)
{
	return input_power < 0.01f;
}

bool is_walking(float input_power)
{
	return input_power >= 0.01f && input_power < 0.5f;
}

bool is_running(float input_power)
{
	return input_power >= 0.5f;
}

} // namespace logic

struct OnInput {
    float input_power = 0;
};

struct OnJump {
};

struct OnLanded {
    float input_power = 0;
};

auto is_idle    = [](const OnInput& e) { return logic::is_idle(e.input_power); };
auto is_walking = [](const OnInput& e) { return logic::is_walking(e.input_power); };
auto is_running = [](const OnInput& e) { return logic::is_running(e.input_power); };

auto is_idle_j    = [](const OnLanded& e) { return logic::is_idle(e.input_power); };
auto is_walking_j = [](const OnLanded& e) { return logic::is_walking(e.input_power); };
auto is_running_j = [](const OnLanded& e) { return logic::is_running(e.input_power); };

enum class MovementState {
    Idle,
    Walk,
    Run,
    Jump,
};

struct Context {
	MovementState state {};
	bool is_ground = true;
};

auto is_ground = [](const Context& e) { return e.is_ground; };

namespace st {
struct Idle {};
struct Walk {};
struct Run {};
struct Jump {};
} // namespace st


struct MovementSm {
    auto operator()() const {
        using namespace sml;

        auto to_idle = [](Context& ctx) -> void
        {
        	ctx.state = MovementState::Idle;
        };
        auto to_walk = [](Context& ctx) -> void
        {
        	ctx.state = MovementState::Walk;
        };
        auto to_run = [](Context& ctx) -> void
        {
        	ctx.state = MovementState::Run;
        };
        auto to_jump = [](Context& ctx) -> void
        {
        	ctx.state = MovementState::Jump;
        	ctx.is_ground = false;
        };
        auto to_land = [](Context& ctx) -> void
        {
        	ctx.is_ground = true;
        };

		return make_transition_table(
			*state<st::Idle> + on_entry<_> / to_idle
			,state<st::Walk> + on_entry<_> / to_walk
			,state<st::Run> + on_entry<_> / to_run
			,state<st::Jump> + on_entry<_> / to_jump

            ,state<_> + event<OnJump> [is_ground] = state<st::Jump>

			,state<st::Idle> + event<OnInput> [is_walking] = state<st::Walk>
			,state<st::Idle> + event<OnInput> [is_running] = state<st::Run>

			,state<st::Walk> + event<OnInput> [is_running] = state<st::Run>
			,state<st::Walk> + event<OnInput> [is_idle] = state<st::Idle>

			,state<st::Run> + event<OnInput> [is_walking] = state<st::Walk>
			,state<st::Run> + event<OnInput> [is_idle] = state<st::Idle>

			,state<st::Jump> + event<OnLanded> [is_walking_j] / to_land = state<st::Walk>
			,state<st::Jump> + event<OnLanded> [is_running_j] / to_land = state<st::Run>
			,state<st::Jump> + event<OnLanded> [is_idle_j] / to_land = state<st::Idle>
		);
    }
};


TEST(SmlP2, test1)
{
	Context ctx;
	sml::sm<MovementSm> sm{ctx};
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);
	sm.process_event(OnInput{});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{1.0f});
	ASSERT_EQ(MovementState::Run, ctx.state);
	sm.process_event(OnInput{});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);
	sm.process_event(OnInput{1.0f});
	ASSERT_EQ(MovementState::Run, ctx.state);
	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);
	sm.process_event(OnInput{});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnJump{});
	ASSERT_EQ(MovementState::Jump, ctx.state);
	sm.process_event(OnLanded{0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);

	sm.process_event(OnJump{});
	ASSERT_EQ(MovementState::Jump, ctx.state);
	sm.process_event(OnLanded{1.0f});
	ASSERT_EQ(MovementState::Run, ctx.state);

	sm.process_event(OnJump{});
	ASSERT_EQ(MovementState::Jump, ctx.state);
	sm.process_event(OnLanded{});
	ASSERT_EQ(MovementState::Idle, ctx.state);
}

} // namespace sml_p2
