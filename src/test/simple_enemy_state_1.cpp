// simple_enemy_state_1.cpp
#include <gtest/gtest.h>
#include "simple_player_state.h"

namespace simple_enemy_state_1 {

enum class ShooterView {
	int post_address = 0;
};

#if NOTE
- callする仕組みを作る
- viewにはハンドルが入っている
- ハンドルは単純なタイプである必要がある(intとか)
- しかしコールバックは引数付き
- ポストからコールバックを取り出す関数が必要(昔作ったような・・・)

- torelloにいっぱいタスクを作ろう
- 会社用のwebサービスって簡単に作れる？
#endif

TEST(SimpleEnemyState, test1)
{
	ASSERT_TRUE(false);

#if NOTE
	is_called(post_address);

	callback(post_address, []() {

	});	
#endif
}

} // namespace simple_enemy_state_1
