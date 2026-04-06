#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <functional>

// あああ
namespace test_1 {

struct PlayerState;

enum class PlayerStateType {
    IDLE,
    WALK,
    RUN,
    DASH,
    SPRINT,
    KNOCK_BACK,
};

struct ActionRequest {
    std::string name;
    std::function<void (PlayerState&)> executor;
};

struct ActionQue {
    //std::mutex Mutex;
    std::vector<ActionRequest> que;
    bool is_deprecated = false;

    bool post(ActionRequest&& req)
    {
        //std::lock_guard lock(Mutex);
        //if (!bClosed) Pending.push_back(Req);
        if (is_deprecated)
            return false;
        que.push_back(std::move(req));
        return true;
    }    
};

struct PlayerState {
    PlayerStateType moveState = PlayerStateType::IDLE;
    float knock_back_time = 0;

    std::shared_ptr<ActionQue> p_que_ = std::make_shared<ActionQue>();

    bool is_ok() const
    {
        return knock_back_time == 0;
    }

    bool is_invincible() const
    {
        return moveState == PlayerStateType::KNOCK_BACK;
    }

    bool walk()
    {
        if (!is_ok())
            return false;
        moveState = PlayerStateType::WALK;
        return true;
    }

    bool run()
    {
        if (!is_ok())
            return false;
        moveState = PlayerStateType::RUN;
        return true;
    }

    bool attacked()
    {
        moveState = PlayerStateType::KNOCK_BACK;
        knock_back_time = 1.0f;
        return true;
    }

    void tick(float delta_time)
    {
        if (knock_back_time > 0) {
            knock_back_time -= delta_time;
            if (knock_back_time > 0)
                return;
            knock_back_time = 0;
            if (moveState == PlayerStateType::KNOCK_BACK)
                moveState = PlayerStateType::IDLE;
        }

        if (!p_que_->que.empty()) {
            std::vector<ActionRequest> que = std::move(p_que_->que);
            for (auto& req: que)
                req.executor(*this);
        }


        if (playerView_ != nullptr) {
            if (playerView_->moveState_ != moveState)
                playerView_ = createPlayerView();
        }
    }

    struct PlayerView {
        PlayerStateType moveState_ = PlayerStateType::IDLE;
        std::weak_ptr<ActionQue> p_que_;

        PlayerStateType moveState() const
        {
            return moveState_;
        }

        bool walk()
        {
            if (auto p = p_que_.lock()) {
                ActionRequest req;
                req.name = "walk";
                req.executor = [](auto& s) {
                    s.walk();
                };
                return p->post(std::move(req));
            }
            return false;
        }

        bool run()
        {
            if (auto p = p_que_.lock()) {
                ActionRequest req;
                req.name = "run";
                req.executor = [](auto& s) {
                    s.run();
                };
                return p->post(std::move(req));
            }
            return false;
        }
    };

    mutable std::shared_ptr<PlayerView> playerView_;

    std::shared_ptr<PlayerView> getPlayerView() const
    {
        if (playerView_ == nullptr)
            playerView_ = createPlayerView();
        return playerView_;
    }

    std::shared_ptr<PlayerView> createPlayerView() const
    {
        auto p = std::make_shared<PlayerView>();
        p->moveState_ = moveState;
        p->p_que_ = p_que_;
        return p;
    }
};


TEST(Test1, test1) {
    PlayerState ps;
    ASSERT_EQ(PlayerStateType::IDLE, ps.moveState);

    ASSERT_FALSE(ps.is_invincible());
    ASSERT_TRUE(ps.walk());
    ASSERT_EQ(PlayerStateType::WALK, ps.moveState);

    ASSERT_TRUE(ps.run());
    ASSERT_EQ(PlayerStateType::RUN, ps.moveState);

    ASSERT_TRUE(ps.attacked());
    ASSERT_EQ(PlayerStateType::KNOCK_BACK, ps.moveState);
    ASSERT_TRUE(ps.is_invincible());

    ASSERT_FALSE(ps.walk());
    ps.tick(0.5f);
    ASSERT_TRUE(ps.is_invincible());
    ps.tick(0.5f);

    ASSERT_EQ(PlayerStateType::IDLE, ps.moveState);
    ASSERT_FALSE(ps.is_invincible());

    ASSERT_TRUE(ps.walk());
    ASSERT_EQ(PlayerStateType::WALK, ps.moveState);

    ASSERT_EQ(PlayerStateType::WALK, ps.getPlayerView()->moveState());
    auto p1 = ps.getPlayerView();
    ASSERT_EQ(PlayerStateType::WALK, p1->moveState());
}

TEST(Test1, test2) {
    PlayerState ps;
    ASSERT_EQ(PlayerStateType::IDLE, ps.moveState);

    auto p_view = ps.getPlayerView();
    ASSERT_TRUE(p_view);
    ASSERT_EQ(p_view, ps.getPlayerView());

    ASSERT_EQ(PlayerStateType::IDLE, p_view->moveState());
    ASSERT_TRUE(p_view->walk());
    ASSERT_EQ(PlayerStateType::IDLE, ps.moveState);

    ps.tick(0.1f);
    ASSERT_EQ(PlayerStateType::WALK, ps.moveState);

    ASSERT_EQ(PlayerStateType::IDLE, p_view->moveState());
    ASSERT_NE(p_view, ps.getPlayerView());
    p_view = ps.getPlayerView();
    ASSERT_EQ(PlayerStateType::WALK, p_view->moveState());

}

} // namespace test_1
