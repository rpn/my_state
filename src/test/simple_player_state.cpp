// simple_player_state.cpp
#include "simple_player_state.h"

namespace simple_player_state {

bool ActionQue::post(ActionRequest&& req)
{
    if (is_deprecated)
        return false;
    que.push_back(std::move(req));
    return true;
}    

bool PlayerState::transition(PlayerStateType state)
{
	if (state_ == state)
		return true;

    switch (state) {
    case PlayerStateType::SLIDE:
        if (state_ != PlayerStateType::RUN)
            return false;
        break;
    }
    if (state_ == PlayerStateType::SLIDE) {
        state_ = PlayerStateType::DASH;
        dash_timer_ = 30.0f;
        return true;
    }
	state_ = state;
	return true;
}

bool PlayerState::transition(float delta_time, const MoveIntent& intent)
{
    if (state() == PlayerStateType::DASH) {
        dash_timer_ -= delta_time;
        if (dash_timer_ > 0.0f)
            return false;
        dash_timer_ = 0;
    }

    if (intent.is_slide && transition(PlayerStateType::SLIDE))
        return true;

	if (intent.velocity > 0.5f)
		return transition(PlayerStateType::RUN);

	if (intent.velocity > 0.0f)
		return transition(PlayerStateType::WALK);

	return transition(PlayerStateType::IDLE);
}

void PlayerState::tick(float delta_time, const MoveIntent& intent)
{
    transition(delta_time, intent);

    switch (state()) {
    case PlayerStateType::SLIDE:
        power_ += delta_time;
        break;
    case PlayerStateType::DASH:
        break;
    default:
        power_ = 0;
        break;
    }

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

float PlayerState::speed() const
{
    switch (state()) {
    case PlayerStateType::WALK:
	    return 600.f;
    case PlayerStateType::RUN:
	    return 1200.f;
    case PlayerStateType::SLIDE:
	    return 800.f;
    case PlayerStateType::DASH:
	    return 5000.f;
    }
    return 0;
}

std::shared_ptr<PlayerState::PlayerView> PlayerState::player_view() const
{
    if (player_view_ == nullptr)
        player_view_ = create_player_view();
    return player_view_;
}

std::shared_ptr<PlayerState::PlayerView> PlayerState::create_player_view() const
{
    auto p = std::make_shared<PlayerView>();
    p->state_ = state_;
    p->p_que_ = p_que_;
    p->speed_ = speed();
    return p;
}

} // namespace simple_player_state
