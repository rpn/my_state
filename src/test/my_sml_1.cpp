// my_sml_1.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace my_sml_1 {
namespace sml = boost::sml;

struct on_update {
    float delta_time;
    float input_power;
    bool jump = false;
};

auto is_idle    = [](const on_update& e) { return e.input_power < 0.1f; };
auto is_walking = [](const on_update& e) { return e.input_power >= 0.1f && e.input_power < 0.8f; };
auto is_running = [](const on_update& e) { return e.input_power >= 0.8f; };
auto is_jumping = [](const on_update& e) { return e.jump == true; };

enum class MovementState {
	Idle,
	Walk,
	Run,
	Jump,
};

struct Context {
	MovementState state = MovementState::Idle;
};

// idle/walk/run を地上移動サブマシンに閉じ込める。
// Jump 遷移は親で一度だけ定義することで重複を防ぐ。
struct grounded_sm {
    auto operator()() const {
        using namespace sml;

        auto to_idle = [](Context& ctx) { ctx.state = MovementState::Idle; };
        auto to_walk = [](Context& ctx) { ctx.state = MovementState::Walk; };
        auto to_run = [](Context& ctx) { ctx.state = MovementState::Run; };
        
        return make_transition_table(
	        *"idle"_s + on_entry<_> / to_idle,
	        "walk"_s + on_entry<_> / to_walk,
	        "run"_s + on_entry<_> / to_run,

	        *"idle"_s + event<on_update> [is_walking] = "walk"_s,
	        "idle"_s + event<on_update> [is_running] = "run"_s,
	        "walk"_s + event<on_update> [is_running] = "run"_s,
	        "walk"_s + event<on_update> [is_idle] = "idle"_s,
	        "run"_s + event<on_update> [is_walking] = "walk"_s,
	        "run"_s + event<on_update> [is_idle] = "idle"_s
        );
    }
};

struct character_movement {
    auto operator()() const {
        using namespace sml;

		auto to_jump = [](Context& ctx) { ctx.state = MovementState::Jump; };

        return make_transition_table(
	        *state<grounded_sm> + event<on_update> [is_jumping] / to_jump = "jump"_s,
	        "jump"_s + event<on_update> [is_idle] = state<grounded_sm>
        );
    }
};

TEST(MySml1, test1)
{
	Context ctx;
	sml::sm<character_movement> sm{ctx};

	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(on_update{1.0f, 0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);

	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(on_update{1.0f, 1.0f});
	ASSERT_EQ(MovementState::Run, ctx.state);

	sm.process_event(on_update{1.0f, 0.2f});
	ASSERT_EQ(MovementState::Walk, ctx.state);

	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(on_update{1.0f, 0.0f, true});
	ASSERT_EQ(MovementState::Jump, ctx.state);
	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(on_update{1.0f, 1.0f});
	ASSERT_EQ(MovementState::Run, ctx.state);

	sm.process_event(on_update{1.0f, 1.0f, true});
	ASSERT_EQ(MovementState::Jump, ctx.state);
}

} // namespace my_sml_1
