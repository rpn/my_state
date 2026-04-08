// sml_test_3.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_test_3 {
namespace sml = boost::sml;

// 1. イベントの定義
struct on_update {
	float delta_time;
};

// 2. 外部データ（タイマー）の定義
struct Timer {
	float elapsed = 0.0f;
};

// 3. アクションとガードの定義
// イベントからdelta_timeを受け取り、注入されたTimerに加算する
auto tick = [](const on_update& e, Timer& t) {
	t.elapsed += e.delta_time;
};

// 加算後の値（elapsed + delta_time）が指定秒数に達するかを判定するガード
auto is_timeout = [](const on_update& e, const Timer& t) {
	return (t.elapsed + e.delta_time) >= 3.0f;
};

// タイマーをリセットするアクション（遷移時に実行）
auto reset_timer = [](Timer& t) {
	t.elapsed = 0.0f;
};

// 4. ステートマシンの定義
struct timeout_machine {
	auto operator()() const {
		using namespace sml;
		return make_transition_table(
			// 評価順序は記述した順になります
			
			// ① イベント反映後に3秒到達するなら "next_state" へ遷移し、タイマーをリセット
			*"waiting"_s + event<on_update> [is_timeout] / reset_timer = "next_state"_s,
			
			// ② 経過していなければ（上の条件から漏れたら）、時間を加算する（遷移はしない）
			 "waiting"_s + event<on_update>              / tick
		);
	}
};


TEST(SmlTest3, test1)
{
	using sml::literals::operator""_s;	

	// 外部データのインスタンス化
	Timer my_timer;
	
	// ステートマシンにTimerを注入
	sml::sm<timeout_machine> sm{my_timer};	

	sm.process_event(on_update{0.5f});
	ASSERT_TRUE(sm.is("waiting"_s));
	ASSERT_EQ(0.5f, my_timer.elapsed);

	sm.process_event(on_update{0.5f});
	ASSERT_TRUE(sm.is("waiting"_s));
	ASSERT_EQ(1.0f, my_timer.elapsed);

	sm.process_event(on_update{100.0f});
	ASSERT_TRUE(sm.is("next_state"_s));
	ASSERT_EQ(0.0f, my_timer.elapsed);
}

} // namespace sml_test_3
