// my_sml_2.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace my_sml_2 {
namespace sml = boost::sml;


struct OnInput {
	float delta_time;
	float input_power;
	bool jump = false;
};

struct Context {
	float speed {};
};

auto is_idle    = [](const OnInput& e) { return e.input_power < 0.1f; };
auto is_walking = [](const OnInput& e) { return e.input_power >= 0.1f && e.input_power < 0.8f; };
auto is_running = [](const OnInput& e) { return e.input_power >= 0.8f; };
auto is_jumping = [](const OnInput& e) { return e.jump == true; };

struct GroundSm {
	auto operator()() const {
		using namespace sml;
		
		return make_transition_table(
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

		return make_transition_table(
			*state<GroundSm> + event<OnInput> [is_jumping] = "jump"_s,

			"jump"_s + event<OnInput> [is_running] = state<GroundSm>,
			"jump"_s + event<OnInput> [is_walking] = state<GroundSm>,
			"jump"_s + event<OnInput> [is_idle] = state<GroundSm>
		);
	}
};

struct Player: Context {
	sml::sm<MovementSm> sm;
	Player() = default;

	void update(float delta_time, float input_power, bool jump)
	{
		speed = input_power;
		sm.process_event(OnInput{delta_time, input_power, jump});
	}
};

TEST(MySml2, test1)
{
	using sml::literals::operator""_s;

	Context ctx;
	ASSERT_EQ(0.0f, ctx.speed);

	// GroundSm 単体で idle/walk/run の遷移を確認する。
	sml::sm<GroundSm> ground_sm;
	ASSERT_TRUE(ground_sm.is("idle"_s));
	ground_sm.process_event(OnInput{1.0f, 0.1f, false});
	ASSERT_TRUE(ground_sm.is("walk"_s));
	ground_sm.process_event(OnInput{1.0f, 1.0f, false});
	ASSERT_TRUE(ground_sm.is("run"_s));
	ground_sm.process_event(OnInput{1.0f, 0.0f, false});
	ASSERT_TRUE(ground_sm.is("idle"_s));

	Player pl;
	ASSERT_FALSE(pl.sm.is("jump"_s));
	//ASSERT_EQ(0.0f, ctx.speed);

	pl.update(1.0f, 0.1f, false);
	ASSERT_FALSE(pl.sm.is("jump"_s));

	pl.update(1.0f, 0.1f, true);
	ASSERT_TRUE(pl.sm.is("jump"_s));

	pl.update(1.0f, 0.1f, false);
	ASSERT_FALSE(pl.sm.is("jump"_s));
}

} // namespace my_sml_2
