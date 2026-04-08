// simple_player_state.h
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace simple_player_state {

struct PlayerState;

struct ActionRequest {
    std::string name;
    std::function<void (PlayerState&)> executor;
};

struct ActionQue {
    std::vector<ActionRequest> que;
    bool is_deprecated = false;
    bool post(ActionRequest&& req);
};

enum class PlayerStateType {
    IDLE,
    WALK,
    RUN,
    SLIDE,
    DASH,
};

struct MoveIntent {
    float velocity;
    bool is_slide;
};

struct PlayerState {
    PlayerStateType state_ = PlayerStateType::IDLE;
    float power_ = 0;
    float dash_timer_ = 0;
    std::shared_ptr<ActionQue> p_que_ = std::make_shared<ActionQue>();

    PlayerStateType state() const
    {
    	return state_;
    }

    float speed() const;

    bool transition(PlayerStateType state);
    bool transition(float delta_time, const MoveIntent& intent);
    void tick(float delta_time, const MoveIntent& intent);

    struct PlayerView {
        PlayerStateType state_ = PlayerStateType::IDLE;
        std::weak_ptr<ActionQue> p_que_;
        float speed_;

        PlayerStateType state() const
        {
            return state_;
        }

        float speed() const
        {
            return speed_;
        }
    };

    mutable std::shared_ptr<PlayerView> player_view_;

    std::shared_ptr<PlayerView> player_view() const;
    std::shared_ptr<PlayerView> create_player_view() const;
};

} // namespace simple_player_state
