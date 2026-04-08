// sml_test_2.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_test_2 {
namespace sml = boost::sml;

// 1. イベントの定義: 入力情報として float input_power を持たせます [1]
struct on_update {
    float input_power;
};

// 2. ガード（条件）の定義: イベントを受け取り boolean を返します [2, 4]
// ここでは閾値を設定して状態を判定します
auto is_idle    = [](const on_update& e) { return e.input_power < 0.1f; };
auto is_walking = [](const on_update& e) { return e.input_power >= 0.1f && e.input_power < 0.8f; };
auto is_running = [](const on_update& e) { return e.input_power >= 0.8f; };

// 3. アクションの定義: 遷移が成功した際に実行される処理です [4]
auto start_walk = [] { std::cout << "Transitioned to WALK\n"; };
auto start_run  = [] { std::cout << "Transitioned to RUN\n"; };
auto stop       = [] { std::cout << "Transitioned to IDLE\n"; };

// 4. ステートマシン本体（トランジションテーブル）の定義
struct character_movement {
    auto operator()() const {
        using namespace sml;
        
        // 宣言的なUML風の構文で「何をするか」を記述します [5, 6]
        // 構文: 現在の状態 + イベント [ガード条件] / アクション = 次の状態 [4, 7]
        return make_transition_table(
            // 初期状態(IDLE)からの遷移 (* を付けると初期状態になります) [5]
            *"idle"_s + event<on_update> [is_walking] / start_walk = "walk"_s,
             "idle"_s + event<on_update> [is_running] / start_run  = "run"_s,

            // WALK状態からの遷移
             "walk"_s + event<on_update> [is_running] / start_run  = "run"_s,
             "walk"_s + event<on_update> [is_idle]    / stop       = "idle"_s,

            // RUN状態からの遷移
             "run"_s  + event<on_update> [is_walking] / start_walk = "walk"_s,
             "run"_s  + event<on_update> [is_idle]    / stop       = "idle"_s
        );
    }
};

TEST(SmlTest2, test1)
{
	using sml::literals::operator""_s;	

	sml::sm<character_movement> player_sm;
	player_sm.process_event(on_update{0.0f}); // 何も起こらない (すでにIDLE)

	ASSERT_TRUE(player_sm.is("idle"_s));
	//ASSERT_TRUE(player_sm.is(sml::state<idle>));

    player_sm.process_event(on_update{0.5f}); // is_walking が true になり WALK に遷移	
	ASSERT_TRUE(player_sm.is("walk"_s));

    player_sm.process_event(on_update{1.0f}); // is_walking が true になり WALK に遷移	
	ASSERT_TRUE(player_sm.is("run"_s));

    player_sm.process_event(on_update{0.0f}); // is_walking が true になり WALK に遷移	
	ASSERT_TRUE(player_sm.is("idle"_s));
}

} // namespace sml_test_2
