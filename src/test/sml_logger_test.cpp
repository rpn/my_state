// sml_logger_test.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_test_base {
namespace sml = boost::sml;

// ==========================================
// 1. ロガー構造体の定義
// ==========================================
struct my_logger {
	// ① イベントが処理されようとした時に呼ばれる
	template <class SM, class TEvent>
	void log_process_event(const TEvent&) {
		printf("[%s][process_event] %s\n", 
			sml::aux::get_type_name<SM>(), 
			sml::aux::get_type_name<TEvent>());
	}

	// ② ガード条件が評価された結果をログに出す
	template <class SM, class TGuard, class TEvent>
	void log_guard(const TGuard&, const TEvent&, bool result) {
		printf("[%s][guard] %s %s %s\n", 
			sml::aux::get_type_name<SM>(), 
			sml::aux::get_type_name<TGuard>(),
			sml::aux::get_type_name<TEvent>(), 
			(result ? "[OK]" : "[Reject]"));
	}

	// ③ アクションが実行された時に呼ばれる
	template <class SM, class TAction, class TEvent>
	void log_action(const TAction&, const TEvent&) {
		printf("[%s][action] %s %s\n", 
			sml::aux::get_type_name<SM>(), 
			sml::aux::get_type_name<TAction>(),
			sml::aux::get_type_name<TEvent>());
	}

	// ④ 状態が実際に遷移した時に呼ばれる
	template <class SM, class TSrcState, class TDstState>
	void log_state_change(const TSrcState& src, const TDstState& dst) {
		// src と dst には状態名を文字列として取得できる c_str() が用意されています
		printf("[%s][transition] %s -> %s\n", 
			sml::aux::get_type_name<SM>(), 
			src.c_str(), 
			dst.c_str());
	}
};

// ==========================================
// ステートマシンとイベントの定義 (ダミー)
// ==========================================
struct OnJump {};
struct idle {};
struct jump {};

struct PlayerSm {
	auto operator()() const {
		using namespace sml;
		auto play_sound = []() { /* 音を鳴らす */ };
		return make_transition_table(
			*state<idle> + event<OnJump> / play_sound = state<jump>
		);
	}
};


TEST(SmlLogger, test1)
{
	// ロガーのインスタンスを生成
	my_logger logger;

	// ステートマシンの第2テンプレート引数に `sml::logger<my_logger>` を指定し、
	// コンストラクタにロガーのインスタンスを渡す
	sml::sm<PlayerSm, sml::logger<my_logger>> player_sm{logger};
	ASSERT_TRUE(player_sm.is(sml::state<idle>));

	// イベントを発行すると、詳細なログが出力されます
	player_sm.process_event(OnJump{});

	ASSERT_TRUE(player_sm.is(sml::state<jump>));
}

} // namespace sml_test_base
