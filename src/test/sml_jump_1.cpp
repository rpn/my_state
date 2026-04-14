// sml_jump_1.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_test_base {
namespace sml = boost::sml;

#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml = boost::sml;

// ==========================================
// 1. イベントの定義
// ==========================================
struct OnJump {};
// 毎フレームの経過時間を渡すイベント
struct OnTick { float delta_time = 0.0f; }; 

// ==========================================
// 2. コンテキスト（外部データ）
// ==========================================
struct Context {
	float jump_timer = 0.0f;
};

// ==========================================
// 3. 状態型の定義
// ==========================================
struct idle {};
struct jump {};

namespace tr1 {

// ==========================================
// 4. ステートマシン
// ==========================================
struct PlayerSm {
	auto operator()() const {
		using namespace sml;

		// アクション: タイマーの操作 (MSVC対応のため -> void を明記)
		auto reset_timer  = [](Context& ctx) -> void { ctx.jump_timer = 0.0f; };
		auto update_timer = [](const OnTick& e, Context& ctx) -> void { ctx.jump_timer += e.delta_time; };

		// ガード: 1秒経過したか
		auto is_time_up = [](const Context& ctx) { return ctx.jump_timer >= 1.0f; };

		return make_transition_table(
			*state<idle> + event<OnJump> = state<jump>

			// ジャンプ状態に入った瞬間に、on_entryアクションでタイマーをリセット [3, 4]
			,state<jump> + on_entry<_> / reset_timer

			// ========================================================
			// 【重要】毎フレームの処理（上から順に評価される）
			// ========================================================
			// 1. 1秒経過していたら idle に遷移（着地）
			,state<jump> + event<OnTick> [is_time_up] = state<idle>

			// 2. 1秒経過していなければ、タイマーを進める（内部遷移）
			// SMLでは遷移先（= state<xxx>）を省略すると、状態を維持したままアクションのみを実行する
			// 内部遷移（Internal transition）となります [5, 6]。
			,state<jump> + event<OnTick> / update_timer
		);
	}
};

} // namespace tr1

// ==========================================
// GoogleTest スイートによる動作確認
// ==========================================
TEST(SmlJump, JumpAndLandAfterOneSecond) {
	Context ctx;
	sml::sm<tr1::PlayerSm> player_sm{ctx};

	// 1. 初期状態は Idle
	EXPECT_TRUE(player_sm.is(sml::state<idle>));

	// 2. ジャンプ開始 (idle -> jump)
	player_sm.process_event(OnJump{});
	EXPECT_TRUE(player_sm.is(sml::state<jump>));
	EXPECT_EQ(ctx.jump_timer, 0.0f); // on_entryで0にリセットされている

	// 3. 0.5秒経過
	player_sm.process_event(OnTick{0.5f});
	EXPECT_TRUE(player_sm.is(sml::state<jump>)); // まだ1秒未満なのでjumpのまま
	EXPECT_EQ(ctx.jump_timer, 0.5f);

	// 4. さらに0.6秒経過（合計1.1秒）
	// OnTickが送信された際、上の行のガード[is_time_up]が真になるため、idleへ遷移する。
	player_sm.process_event(OnTick{0.6f});
	EXPECT_TRUE(player_sm.is(sml::state<idle>)); // 1秒を超えたので着地！
}

} // namespace sml_test_base
