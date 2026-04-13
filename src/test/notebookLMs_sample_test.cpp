#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace notebookLMs_sample_test {
namespace sml = boost::sml;

enum class MovementState {
    Idle,
    Walk,
    Jump,
};

struct Context {
    MovementState state = MovementState::Idle;
    // 無名遷移が参照できるように、最新の入力状態を保持しておく変数
    float current_input_power = 0.0f; 
};

struct OnInput {
    float input_power = 0.0f;
    bool jump = false;
};

// ==========================================
// 1. ガード条件の定義
// ==========================================
// 通常のイベント駆動用ガード
auto is_walking = [](const OnInput& e) { return e.input_power > 0.01f && e.input_power < 1.0f; };
auto is_idle    = [](const OnInput& e) { return e.input_power <= 0.01f; };
auto is_jumping = [](const OnInput& e) { return e.jump; };

// 無名遷移用ガード（イベントがないため、Contextに保存された最新の入力を評価する）
auto is_walking_ctx = [](const Context& ctx) { return ctx.current_input_power > 0.01f; };
auto is_idle_ctx    = [](const Context& ctx) { return ctx.current_input_power <= 0.01f; };

// ==========================================
// 2. アクションの定義
// ==========================================
// 遷移時に送られてきたイベントの入力値をContextに保存するアクション
auto save_input = [](const OnInput& e, Context& ctx) -> void {
    ctx.current_input_power = e.input_power;
};

// ==========================================
// 状態型の定義 (MSVC完全対応)
// ==========================================
struct landing_hub {}; // ダミーの初期状態（着地のハブ）
struct idle {};
struct walk {};
struct jump {};

namespace tr1 {

// ==========================================
// サブステート（無名遷移による即時ルーティング）
// ==========================================
struct GroundMovementSm {
    auto operator()() const {
        using namespace sml;

        auto to_idle = [](Context& ctx) -> void { ctx.state = MovementState::Idle; };
        auto to_walk = [](Context& ctx) -> void { ctx.state = MovementState::Walk; };
        
        return make_transition_table(
            // 【重要】ダミーの初期状態からの「無名遷移」
            // SMLの仕様により、イベント(<OnInput>など)を記述しない遷移は無名遷移となります [1]。
            // サブステートに入った瞬間、以下のガードを即座に評価してWalkかIdleに分岐します。
            (*state<landing_hub>) [is_walking_ctx] = state<walk>
            ,state<landing_hub> [is_idle_ctx]    = state<idle>

            // 通常の入場アクション
            ,state<idle> + on_entry<_> / to_idle
            ,state<walk> + on_entry<_> / to_walk

            // 地上での状態遷移（入力値を保存しつつ遷移）
            ,state<idle> + event<OnInput> [is_walking] / save_input = state<walk>
            ,state<walk> + event<OnInput> [is_idle]    / save_input = state<idle>
        );
    }
};

// ==========================================
// メインステートマシン（全体管理）
// ==========================================
struct MainSm {
    auto operator()() const {
        using namespace sml;

        auto to_jump = [](Context& ctx) -> void { ctx.state = MovementState::Jump; };

        return make_transition_table(
            // サブステートからジャンプへの一括遷移
            *state<GroundMovementSm> + event<OnInput> [is_jumping] / save_input = state<jump>
            
            ,state<jump> + on_entry<_> / to_jump

            // 【重要】ジャンプ終了後、地上のサブステートへ戻る
            // アクション(save_input)を使って、着地時の入力(OnInput)をContextに保存してから
            // GroundMovementSmの初期状態(landing_hub)へ復帰させます。
            ,state<jump> + event<OnInput> [!is_jumping] / save_input = state<GroundMovementSm>
        );
    }
};

} // namespace tr1

// ==========================================
// GoogleTest スイート
// ==========================================
TEST(SmlStateMachineTest, SubstateAnonymousTransition) {
    Context ctx;
    sml::sm<tr1::MainSm> player_sm{ctx};

    // 1. 初期状態 (landing_hub -> 無名遷移 -> idle)
    EXPECT_EQ(ctx.state, MovementState::Idle);

    // 2. 歩き出し (idle -> walk)
    player_sm.process_event(OnInput{0.5f, false});
    EXPECT_EQ(ctx.state, MovementState::Walk);

    // 3. ジャンプ (walk -> jump)
    player_sm.process_event(OnInput{0.5f, true});
    EXPECT_EQ(ctx.state, MovementState::Jump);

    // 4. 着地して「そのまま歩き続ける」(jump -> landing_hub -> 即座にwalk)
    // ここが今回の肝です。process_eventは1回しか呼び出しませんが、
    // 内部の無名遷移によって1フレームも待たずに直接Walkに入ります。
    player_sm.process_event(OnInput{0.5f, false});
    EXPECT_EQ(ctx.state, MovementState::Walk);

    // 5. 着地して「立ち止まる」(jump -> landing_hub -> 即座にidle)
    player_sm.process_event(OnInput{0.5f, true}); // 再度ジャンプ
    EXPECT_EQ(ctx.state, MovementState::Jump);

    player_sm.process_event(OnInput{0.0f, false}); // 停止状態で着地
    EXPECT_EQ(ctx.state, MovementState::Idle);
}
} // namespace notebookLMs_sample_test


