// sml_p4.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_p4 {
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

enum class PlayerState {
	Idle,
	Walk,
	Run,
	Dash,
	Jump,
	Damage,
	Dead,
};

namespace st {
struct Idle {};
struct Walk {};
struct Run {};
struct Dash {};
struct Jump {};
struct Damage {};
} // namespace st


struct OnInput {
	float input_power = 0;
};

struct PushJump {
};

struct OnLanded {
};

struct PushDash {
};

struct ReleaseDash {
};

struct Attack {
	int damage {};
};

struct Tick {
	float delta_time {};
};

struct Context {
	PlayerState state {};
	bool is_ground = true;

	int hp = 1;
	float damage_time {};
};

auto is_idle    = [](const OnInput& e) { return logic::is_idle(e.input_power); };
auto is_walking = [](const OnInput& e) { return logic::is_walking(e.input_power); };
auto is_running = [](const OnInput& e) { return logic::is_running(e.input_power); };
auto is_ground	= [](const Context& e) { return e.is_ground; };
auto is_recover = [](const Context& e) { return e.damage_time == 0; };

auto to_idle = [](Context& ctx) -> void
{
	ctx.state = PlayerState::Idle;
};
auto to_walk = [](Context& ctx) -> void
{
	ctx.state = PlayerState::Walk;
};
auto to_run = [](Context& ctx) -> void
{
	ctx.state = PlayerState::Run;
};

struct GroundSm {
	auto operator()() const {
		using namespace sml;

		return make_transition_table(
			*state<st::Idle> + on_entry<_> / to_idle
			,state<st::Walk> + on_entry<_> / to_walk
			,state<st::Run> + on_entry<_> / to_run

			,state<st::Idle> + event<OnInput> [is_walking] = state<st::Walk>
			,state<st::Idle> + event<OnInput> [is_running] = state<st::Run>

			,state<st::Walk> + event<OnInput> [is_running] = state<st::Run>
			,state<st::Walk> + event<OnInput> [is_idle] = state<st::Idle>

			,state<st::Run> + event<OnInput> [is_walking] = state<st::Walk>
			,state<st::Run> + event<OnInput> [is_idle] = state<st::Idle>
		);
	}
};

auto to_jump = [](Context& ctx) -> void
{
	ctx.state = PlayerState::Jump;
	ctx.is_ground = false;
};

auto to_land = [](Context& ctx) -> void
{
	ctx.is_ground = true;
};

auto to_dash = [](Context& ctx) -> void
{
	ctx.state = PlayerState::Dash;
};

auto to_damage = [](Context& ctx) -> void
{
	ctx.state = PlayerState::Damage;
	ctx.damage_time = 3.0f;
};

auto recover = [](Context& ctx, const Tick& tick) -> void
{
	if (ctx.damage_time > 0.0f)
		ctx.damage_time -= tick.delta_time;
	if (ctx.damage_time < 0.0f)
		ctx.damage_time = 0;
};


struct MovementSm {
	auto operator()() const {
		using namespace sml;

		return make_transition_table(
			*state<GroundSm> + event<PushJump> = state<st::Jump>
			,state<GroundSm> + event<Attack> = state<st::Damage>

			,state<st::Jump> + on_entry<_> / to_jump
			,state<st::Damage> + on_entry<_> / to_damage

			,state<st::Jump> + event<OnLanded> / to_land = state<GroundSm>
			,state<st::Damage> + event<Tick> / recover
			,state<st::Damage> + event<PushJump> [is_recover] = state<GroundSm> 
		);
	}
};

struct DamageSm {
	auto operator()() const {
		using namespace sml;

		return make_transition_table(
		);
	}
};


TEST(SmlP4, test1)
{
	Context ctx;
	sml::sm<GroundSm> sm{ctx};
	ASSERT_EQ(PlayerState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(PlayerState::Walk, ctx.state);

	sm.process_event(OnInput{1.0f});
	ASSERT_EQ(PlayerState::Run, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(PlayerState::Walk, ctx.state);

	sm.process_event(OnInput{});
	ASSERT_EQ(PlayerState::Idle, ctx.state);

	sm.process_event(PushJump{});
	ASSERT_EQ(PlayerState::Idle, ctx.state);
}

TEST(SmlP4, test2)
{
	Context ctx;
	sml::sm<MovementSm> sm{ctx};
	ASSERT_EQ(PlayerState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(PlayerState::Walk, ctx.state);

	sm.process_event(OnInput{1.0f});
	ASSERT_EQ(PlayerState::Run, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(PlayerState::Walk, ctx.state);

	sm.process_event(OnInput{});
	ASSERT_EQ(PlayerState::Idle, ctx.state);
	ASSERT_TRUE(ctx.is_ground);

	sm.process_event(PushJump{});
	ASSERT_EQ(PlayerState::Jump, ctx.state);
	ASSERT_FALSE(ctx.is_ground);

	sm.process_event(OnLanded{});
	ASSERT_EQ(PlayerState::Idle, ctx.state);
	ASSERT_TRUE(ctx.is_ground);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(PlayerState::Walk, ctx.state);

	sm.process_event(PushJump{});
	ASSERT_EQ(PlayerState::Jump, ctx.state);
	ASSERT_FALSE(ctx.is_ground);
}

TEST(SmlP4, damage)
{
	Context ctx;
	sml::sm<MovementSm> sm{ctx};
	ASSERT_EQ(PlayerState::Idle, ctx.state);

	sm.process_event(Attack{1});
	ASSERT_EQ(PlayerState::Damage, ctx.state);
	ASSERT_EQ(3.f, ctx.damage_time);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(PlayerState::Damage, ctx.state);

	sm.process_event(Tick{10.f});
	ASSERT_EQ(0.f, ctx.damage_time);
	ASSERT_EQ(PlayerState::Damage, ctx.state);

	sm.process_event(PushJump{});
	ASSERT_EQ(PlayerState::Idle, ctx.state);

	sm.process_event(OnInput{1.0f});
	ASSERT_EQ(PlayerState::Run, ctx.state);
}

}
