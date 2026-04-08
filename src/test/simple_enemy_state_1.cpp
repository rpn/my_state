// simple_enemy_state_1.cpp
#include <gtest/gtest.h>
#include "simple_player_state.h"

namespace simple_enemy_state_1 {

#if NOTE
enum class ShooterView {
	int post_address = 0;

	bool open_post_box();
		// 開設
	void close_post_box();
};
#endif

#if NOTE
- callする仕組みを作る
- viewにはハンドルが入っている
- ハンドルは単純なタイプである必要がある(intとか)
- しかしコールバックは引数付き
- ポストからコールバックを取り出す関数が必要(昔作ったような・・・)
- 受け取りがなかったらどうなる？キューにたまり続ける？

開設したら届くようになる
	- 開設以前のイベントが届かないのは困る場合は？
		- 強制開設があってもいいかも？
	- 複数の相手に同時に届けるとかできるの？
		- 便利関数としては用意するかも

- torelloにいっぱいタスクを作ろう
- 会社用のwebサービスって簡単に作れる？
#endif

TEST(SimpleEnemyState, test1)
{
	ASSERT_TRUE(true);

#if NOTE
	is_called(post_address);

	callback(post_address, []() {

	});	
#endif
}

} // namespace simple_enemy_state_1
