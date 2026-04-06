// test_2.cpp
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <functional>

// あああ
namespace sub_state_test_1 {

struct PlayerState;

struct LocomotionState {
	enum class Type {
	    IDLE,
	    WALK,
	    RUN,
	    DASH,
	    STUNNED // 親（PlayerState）から強制される状態
	};

	Type state_ = Type::IDLE;

	Type state() const
	{
		return state_;
	}

    bool is_ok() const
    {
        return state_ == Type::STUNNED;
    }

    bool transion(Type state)
    {
    	if (state_ == state)
    		return true;
    	if (!is_ok())
    		return false;
    	state_ = state;
    	return true;
    }
};

struct ActionRequest {
    std::string name;
    std::function<void (PlayerState&)> executor;
};

struct ActionQue {
    std::vector<ActionRequest> que;
    bool is_deprecated = false;

    bool post(ActionRequest&& req)
    {
        if (is_deprecated)
            return false;
        que.push_back(std::move(req));
        return true;
    }    
};

struct MoveIntent {
    float velocity;
    bool is_jump_pressed;
    bool is_dash_pressed;
};

struct PlayerState {
	enum class Type {
	    IDLE,
	    MOVE,
	    JUMP,
	    KNOCK_BACK,
	};
    Type state_ = Type::IDLE;
   	LocomotionState locomotion_;

    float knock_back_time_ = 0;
    std::shared_ptr<ActionQue> p_que_ = std::make_shared<ActionQue>();

   	Type state() const
   	{
   		return state_;
   	}

    bool transion(Type state)
    {
    	if (state_ == state)
    		return true;
    	state_ = state;
    	return true;
    }

    void tick(float delta_time, const MoveIntent& intent)
    {
    	if (intent.velocity > 5.0f)
    		locomotion_.transion(LocomotionState::Type::WALK);
    	else if (intent.velocity > 1.0f)
    		locomotion_.transion(LocomotionState::Type::WALK);
    	else 
    		locomotion_.transion(LocomotionState::Type::IDLE);
    }
};


#if 0
struct PlayerState {
    PlayerStateType move_state_ = PlayerStateType::IDLE;
    float knock_back_time_ = 0;
    std::shared_ptr<ActionQue> p_que_ = std::make_shared<ActionQue>();

    bool is_ok() const
    {
        return knock_back_time_ == 0;
    }

    bool is_invincible() const
    {
        return move_state_ == PlayerStateType::KNOCK_BACK;
    }

    bool walk()
    {
        if (!is_ok())
            return false;
        move_state_ = PlayerStateType::WALK;
        return true;
    }

    bool run()
    {
        if (!is_ok())
            return false;
        move_state_ = PlayerStateType::RUN;
        return true;
    }

    void tick(float delta_time)
    {
    }
};

TEST(Test2, test1) {
	ASSERT_TRUE(false);
}
#endif

} // namespace sub_state_test_1

