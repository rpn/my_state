// state_machine_test_1.cpp
#include <gtest/gtest.h>

namespace state_machine_test_1 {

enum class PlayerStateType {
	IDLE,
	WALK,
	RUN,
	SLIDE,
	DASH,
};

struct ViewBase {
	//std::weak_ptr<ActionQue> p_que_;
};

struct PlayerView: ViewBase {
	PlayerStateType state = PlayerStateType::IDLE;
	float speed = 0;
	bool is_invincible = false;

	
};


struct StateMachineBase {
	virtual bool update() = 0;
};

template <typename T>
struct StateMachine: StateMachineBase {
	T state {};
	bool update() override
	{
		return false;
	}
};


enum class Foo {
	FOO,
	BAR,
	BAZ
};

TEST(StateMachineTest1, test1)
{
	StateMachine<Foo> fm;
	ASSERT_EQ(Foo::FOO, fm.state);
}

} // namespace state_machine_test_1
