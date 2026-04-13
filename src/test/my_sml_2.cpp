// my_sml_2.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>
#include <string_view>

namespace my_sml_2 {
namespace sml = boost::sml;


struct OnInput {
	float delta_time;
	float input_power;
	bool jump = false;
};

struct Context {
	float speed {};
	std::string_view state_name = "idle";
};

auto is_idle    = [](const OnInput& e) { return e.input_power < 0.1f; };
auto is_walking = [](const OnInput& e) { return e.input_power >= 0.1f && e.input_power < 0.8f; };
auto is_running = [](const OnInput& e) { return e.input_power >= 0.8f; };
auto is_jumping = [](const OnInput& e) { return e.jump == true; };

struct GroundSm {
	auto operator()() const {
		using namespace sml;

		auto to_idle = [](Context& pl) { pl.state_name = "idle"; };
		auto to_walk = [](Context& pl) { pl.state_name = "walk"; };
		auto to_run = [](Context& pl) { pl.state_name = "run"; };
		
		return make_transition_table(
			*"idle"_s + on_entry<_> / to_idle,
			"walk"_s + on_entry<_> / to_walk,
			"run"_s + on_entry<_> / to_run,

			*"idle"_s + event<OnInput> [is_walking] = "walk"_s,
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

		auto to_jump = [](Context& pl) { pl.state_name = "jump"; };

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
		speed = input_power;
		sm.process_event(OnInput{delta_time, input_power, jump});
	}

	bool is_walk() const { return state_name == "walk"; }
	bool is_jump() const { return state_name == "jump"; }
	bool is_idle() const { return state_name == "idle"; }
};

TEST(MySml2, test1)
{
	using sml::literals::operator""_s;

	Context ctx;
	ASSERT_EQ(0.0f, ctx.speed);

	// GroundSm 単体で idle/walk/run の遷移を確認する。
	Context ground_ctx;
	sml::sm<GroundSm> ground_sm{ground_ctx};
	ASSERT_TRUE(ground_sm.is("idle"_s));
	ground_sm.process_event(OnInput{1.0f, 0.1f, false});
	ASSERT_TRUE(ground_sm.is("walk"_s));
	ground_sm.process_event(OnInput{1.0f, 1.0f, false});
	ASSERT_TRUE(ground_sm.is("run"_s));
	ground_sm.process_event(OnInput{1.0f, 0.0f, false});
	ASSERT_TRUE(ground_sm.is("idle"_s));

	Player pl;
	ASSERT_FALSE(pl.is_jump());
	//ASSERT_EQ(0.0f, ctx.speed);

	pl.update(1.0f, 0.1f, false);
	ASSERT_TRUE(pl.is_walk());

	pl.update(1.0f, 0.1f, true);
	ASSERT_TRUE(pl.is_jump());

	pl.update(1.0f, 0.1f, false);
	ASSERT_FALSE(pl.is_jump());
	ASSERT_TRUE(pl.is_idle());
}

} // namespace my_sml_2
