// sml_test_3.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>
#include <memory>

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

struct TimerHolder {
	std::unique_ptr<Timer> timer;
};

struct TimeoutMachineContext {
	TimerHolder timer_holder;
};

// 3. アクションとガードの定義
// waitingに入った時だけTimerを確保する
auto allocate_timer = [](TimerHolder& h) {
	if (!h.timer)
		h.timer = std::make_unique<Timer>();
	h.timer->elapsed = 0.0f;
};

// next_stateに入ったらTimerを破棄する
auto release_timer = [](TimerHolder& h) {
	h.timer.reset();
};

// イベントからdelta_timeを受け取り、注入されたTimerに加算する
auto tick = [](const on_update& e, TimerHolder& h) {
	if (h.timer)
		h.timer->elapsed += e.delta_time;
};

// 加算後の値（elapsed + delta_time）が指定秒数に達するかを判定するガード
auto is_timeout = [](const on_update& e, const TimerHolder& h) {
	return h.timer && ((h.timer->elapsed + e.delta_time) >= 3.0f);
};

// 4. ステートマシンの定義
struct timeout_machine {
	auto operator()() const {
		using namespace sml;
		return make_transition_table(
			// waitingに入る時にTimerを確保
			*"waiting"_s + on_entry<_> / allocate_timer,

			// 評価順序は記述した順になります
			
			// ① イベント反映後に3秒到達するなら "next_state" へ遷移
			"waiting"_s + event<on_update> [is_timeout] = "next_state"_s,
			
			// ② 経過していなければ（上の条件から漏れたら）、時間を加算する（遷移はしない）
			"waiting"_s + event<on_update>              / tick,

			// next_stateに入る時にTimerを破棄
			"next_state"_s + on_entry<_> / release_timer
		);
	}
};

using timeout_sm = sml::sm<timeout_machine>;

timeout_sm make_timeout_sm(TimeoutMachineContext& context) {
	return timeout_sm{context.timer_holder};
}


TEST(SmlTest3, test1)
{
	using sml::literals::operator""_s;	

	// 所有は外部にまとめ、依存関係の配線はファクトリに隠蔽する
	TimeoutMachineContext context;
	timeout_sm sm = make_timeout_sm(context);

	ASSERT_TRUE(context.timer_holder.timer);

	sm.process_event(on_update{0.5f});
	ASSERT_TRUE(sm.is("waiting"_s));
	ASSERT_EQ(0.5f, context.timer_holder.timer->elapsed);

	sm.process_event(on_update{0.5f});
	ASSERT_TRUE(sm.is("waiting"_s));
	ASSERT_EQ(1.0f, context.timer_holder.timer->elapsed);

	sm.process_event(on_update{100.0f});
	ASSERT_TRUE(sm.is("next_state"_s));
	ASSERT_FALSE(context.timer_holder.timer);
}

} // namespace sml_test_3
