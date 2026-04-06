// simple_locomotion_test_1.cpp
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <functional>

// あああ
namespace simple_locomotion_test_1 {

struct PlayerState;

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

enum class PlayerStateType {
    IDLE,
    WALK,
    RUN,
};

struct MoveIntent {
    float velocity;
};

struct PlayerState {
    PlayerStateType state_ = PlayerStateType::IDLE;
    std::shared_ptr<ActionQue> p_que_ = std::make_shared<ActionQue>();

    PlayerStateType state() const
    {
    	return state_;
    }

    bool transion(PlayerStateType state)
    {
    	if (state_ == state)
    		return true;
    	state_ = state;
    	return true;
    }

    void tick(float delta_time, const MoveIntent& intent)
    {
    	if (intent.velocity > 5.0f)
    		transion(PlayerStateType::RUN);
    	else if (intent.velocity > 0.0f)
    		transion(PlayerStateType::WALK);
    	else 
    		transion(PlayerStateType::IDLE);

        if (!p_que_->que.empty()) {
            std::vector<ActionRequest> que = std::move(p_que_->que);
            for (auto& req: que)
                req.executor(*this);
        }

        if (player_view_ != nullptr) {
            if (player_view_->state_ != state_)
                player_view_ = create_player_view();
        }
    }

    struct PlayerView {
        PlayerStateType state_ = PlayerStateType::IDLE;
        std::weak_ptr<ActionQue> p_que_;

        PlayerStateType state() const
        {
            return state_;
        }
    };

    mutable std::shared_ptr<PlayerView> player_view_;

    std::shared_ptr<PlayerView> player_view() const
    {
        if (player_view_ == nullptr)
            player_view_ = create_player_view();
        return player_view_;
    }

    std::shared_ptr<PlayerView> create_player_view() const
    {
        auto p = std::make_shared<PlayerView>();
        p->state_ = state_;
        p->p_que_ = p_que_;
        return p;
    }
};


TEST(SimpleLocomotionTest1, test1)
{
	PlayerState ps;
	ASSERT_EQ(PlayerStateType::IDLE, ps.state());

	MoveIntent mi;
	mi.velocity = 0.0f;
	ps.tick(1.0f, mi);

	auto p_view = ps.player_view();
	ASSERT_TRUE(p_view);
	ASSERT_EQ(PlayerStateType::IDLE, p_view->state());

	mi.velocity = 1.0f;
	ps.tick(1.0f, mi);
	ASSERT_EQ(PlayerStateType::WALK, ps.state());
	ASSERT_NE(p_view, ps.player_view());
	p_view = ps.player_view();
	ASSERT_EQ(PlayerStateType::WALK, p_view->state());

	mi.velocity = 10.0f;
	ps.tick(1.0f, mi);
	ASSERT_EQ(PlayerStateType::RUN, ps.state());
	ASSERT_NE(p_view, ps.player_view());
	p_view = ps.player_view();
	ASSERT_EQ(PlayerStateType::RUN, p_view->state());

	mi.velocity = 1.0f;
	ps.tick(1.0f, mi);
	ASSERT_EQ(PlayerStateType::WALK, ps.state());
	ASSERT_NE(p_view, ps.player_view());
	p_view = ps.player_view();
	ASSERT_EQ(PlayerStateType::WALK, p_view->state());

	mi.velocity = 0.0f;
	ps.tick(1.0f, mi);
	ASSERT_EQ(PlayerStateType::IDLE, ps.state());
	ASSERT_NE(p_view, ps.player_view());
	p_view = ps.player_view();
	ASSERT_EQ(PlayerStateType::IDLE, p_view->state());
}

} // namespace simple_locomotion_test_1

