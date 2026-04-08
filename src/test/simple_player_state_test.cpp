// simple_player_state_test.cpp
#include <gtest/gtest.h>
#include "simple_player_state.h"

// あああ
namespace simple_locomotion_test_1 {
using namespace simple_player_state;

TEST(SimplePlayerStateTest1, test1)
{
	PlayerState ps;
	ASSERT_EQ(PlayerStateType::IDLE, ps.state());

	MoveIntent mi;
	mi.velocity = 0.0f;
	ps.tick(1.0f, mi);

	auto p_view = ps.player_view();
	ASSERT_TRUE(p_view);
	ASSERT_EQ(PlayerStateType::IDLE, p_view->state());

	mi.velocity = 0.5f;
	ps.tick(1.0f, mi);
	ASSERT_EQ(PlayerStateType::WALK, ps.state());
	ASSERT_NE(p_view, ps.player_view());
	p_view = ps.player_view();
	ASSERT_EQ(PlayerStateType::WALK, p_view->state());

	mi.velocity = 1.0f;
	ps.tick(1.0f, mi);
	ASSERT_EQ(PlayerStateType::RUN, ps.state());
	ASSERT_NE(p_view, ps.player_view());
	p_view = ps.player_view();
	ASSERT_EQ(PlayerStateType::RUN, p_view->state());

	mi.velocity = 0.5f;
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
