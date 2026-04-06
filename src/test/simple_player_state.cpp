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

bool PlayerState::transion(PlayerStateType state)
{
	if (state_ == state)
		return true;
	state_ = state;
	return true;
}

void PlayerState::tick(float delta_time, const MoveIntent& intent)
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
    return p;
}

} // namespace simple_player_state
