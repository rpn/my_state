// my_sml_3.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace my_sml_3 {
namespace sml = boost::sml;

struct OnInput {
	float delta_time;
	float input_power;
	bool jump = false;
};

auto is_idle    = [](const OnInput& e) {
	return !e.jump && e.input_power < 0.1f;
};
auto is_walking = [](const OnInput& e) {
	return !e.jump && e.input_power >= 0.1f && e.input_power < 0.8f;
};
auto is_running = [](const OnInput& e) {
	return !e.jump && e.input_power >= 0.8f;
};
auto is_jumping = [](const OnInput& e) {
	return e.jump == true;
};

enum class MovementState {
	Idle,
	Walk,
	Run,
	Jump,
};

struct Context {
	MovementState state = MovementState::Idle;
};

struct GroundSm {
	auto operator()() const {
		using namespace sml;

		auto to_idle = [](Context& pl) { pl.state = MovementState::Idle; };
		auto to_walk = [](Context& pl) { pl.state = MovementState::Walk; };
		auto to_run = [](Context& pl) { pl.state = MovementState::Run; };
		
		return make_transition_table(
			*"idle"_s + on_entry<_> / to_idle,
			"walk"_s + on_entry<_> / to_walk,
			"run"_s + on_entry<_> / to_run,

			"idle"_s + event<OnInput> [is_walking] = "walk"_s,
			"idle"_s + event<OnInput> [is_running] = "run"_s,

			"walk"_s + event<OnInput> [is_running] = "run"_s,
			"walk"_s + event<OnInput> [is_idle] = "idle"_s,
		  
			"run"_s + event<OnInput> [is_walking] = "walk"_s,
			"run"_s + event<OnInput> [is_idle] = "idle"_s
		);
	}
};

struct MovementSm {
	auto operator()() const {
		using namespace sml;

		auto to_jump = [](Context& pl) { pl.state = MovementState::Jump; };

		return make_transition_table(
			*state<GroundSm> + event<OnInput> [is_jumping] / to_jump = "jump"_s,

			"jump"_s + event<OnInput> [is_running] = state<GroundSm>,
			"jump"_s + event<OnInput> [is_walking] = state<GroundSm>,
			"jump"_s + event<OnInput> [is_idle] = state<GroundSm>
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
		sm.process_event(OnInput{delta_time, input_power, jump});
	}
};


TEST(SmlTest3, test1)
{
	Player pl;
	ASSERT_EQ(MovementState::Idle, pl.state);

	pl.update(1.0f, 0.1f, false);
	ASSERT_EQ(MovementState::Walk, pl.state);

	pl.update(1.0f, 1.0f, false);
	ASSERT_EQ(MovementState::Run, pl.state);

	pl.update(1.0f, 0.0f, false);
	ASSERT_EQ(MovementState::Idle, pl.state);

	pl.update(1.0f, 0.0f, true);
	ASSERT_EQ(MovementState::Jump, pl.state);

	pl.update(1.0f, 0.0f, false);
	ASSERT_EQ(MovementState::Idle, pl.state);

	pl.update(1.0f, 1.0f, true);
	ASSERT_EQ(MovementState::Jump, pl.state);

	pl.update(1.0f, 0.1f, false);
	ASSERT_EQ(MovementState::Idle, pl.state);

	pl.update(1.0f, 1.0f, false);
	ASSERT_EQ(MovementState::Run, pl.state);

	pl.update(1.0f, 1.0f, true);
	ASSERT_EQ(MovementState::Jump, pl.state);

	pl.update(1.0f, 1.0f, false);
	ASSERT_EQ(MovementState::Run, pl.state);
}

} // namespace my_sml_3
