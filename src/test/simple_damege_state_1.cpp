// simple_damege_state_1.cpp
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <functional>

namespace simple_damage_state_1 {

enum class DamageStateType {
	NONE,
	KNOCK_BACK,
};

struct DamageView {
	DamageStateType state {};
};

struct DamageState {
	DamageStateType state_ {};

	DamageStateType state() const
	{
		return state_;
	}
};



TEST(SimpleDamageState, test2)
{
	ASSERT_TRUE(false);
}

} // namespace simple_damage_state_1
