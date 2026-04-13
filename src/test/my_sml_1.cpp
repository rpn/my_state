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

struct character_movement {
    auto operator()() const {
        using namespace sml;
        
        return make_transition_table(
        	*"idle"_s + event<on_update> [is_walking] = "walk"_s,
        	"idle"_s + event<on_update> [is_running] = "run"_s,

        	"walk"_s + event<on_update> [is_jumping] = "jump"_s,
        	"walk"_s + event<on_update> [is_running] = "run"_s,
        	"walk"_s + event<on_update> [is_idle] = "idle"_s,

        	"run"_s + event<on_update> [is_jumping] = "jump"_s,
        	"run"_s + event<on_update> [is_walking] = "walk"_s,
        	"run"_s + event<on_update> [is_idle] = "idle"_s,

        	"idle"_s + event<on_update> [is_jumping] = "jump"_s,
        	"jump"_s + event<on_update> [is_idle] = "idle"_s
        );
    }
};

TEST(MySml1, test1)
{
    using sml::literals::operator""_s;
	sml::sm<character_movement> sm;

	ASSERT_TRUE(sm.is("idle"_s));

	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_TRUE(sm.is("idle"_s));

	sm.process_event(on_update{1.0f, 0.1f});
	ASSERT_TRUE(sm.is("walk"_s));

	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_TRUE(sm.is("idle"_s));

	sm.process_event(on_update{1.0f, 1.0f});
	ASSERT_TRUE(sm.is("run"_s));

	sm.process_event(on_update{1.0f, 0.2f});
	ASSERT_TRUE(sm.is("walk"_s));

	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_TRUE(sm.is("idle"_s));

	sm.process_event(on_update{1.0f, 0.0f, true});
	ASSERT_TRUE(sm.is("jump"_s));
	sm.process_event(on_update{1.0f, 0.0f});
	ASSERT_TRUE(sm.is("idle"_s));

	sm.process_event(on_update{1.0f, 1.0f});
	ASSERT_TRUE(sm.is("run"_s));

	sm.process_event(on_update{1.0f, 1.0f, true});
	ASSERT_TRUE(sm.is("jump"_s));
}

} // namespace my_sml_1
