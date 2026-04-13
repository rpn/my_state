// sml_any_state.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_any_state {
namespace sml = boost::sml;

enum class MovementState {
    Idle,
    Walk,
    Jump,
};

struct Context {
    MovementState state = MovementState::Idle;
};

struct OnInput {
    float input_power = 0.0f;
    bool jump = false;
};

// ==========================================
// 1. ガード条件の定義
// ==========================================
auto is_walking = [](const OnInput& e) { return e.input_power > 0.01f && e.input_power < 1.0f; };
auto is_idle    = [](const OnInput& e) { return e.input_power <= 0.01f; };
auto is_jumping = [](const OnInput& e) { return e.jump; };

// ★追加：Any State用の特別なガード
// ジャンプ中に再度Any Stateからジャンプが発火（無限ループや再入場）するのを防ぐため、
// 「現在のContextがJumpではない場合のみ」ジャンプを開始できるようにします。
auto is_starting_jump = [](const OnInput& e, const Context& ctx) {
    return e.jump && ctx.state != MovementState::Jump;
};

// ==========================================
// 状態型の定義 (MSVC完全対応)
// ==========================================
struct idle {};
struct walk {};
struct jump {};

namespace tr1 {

// ==========================================
// メインステートマシン（フラット設計）
// ==========================================
struct MainSm {
    auto operator()() const {
        using namespace sml;

        // アクション (MSVC対応) [1]
        auto to_idle = [](Context& ctx) -> void { ctx.state = MovementState::Idle; };
        auto to_walk = [](Context& ctx) -> void { ctx.state = MovementState::Walk; };
        auto to_jump = [](Context& ctx) -> void { ctx.state = MovementState::Jump; };

        return make_transition_table(
            // 1. 各状態への入場アクション定義と初期状態(*)のセット
            *state<idle> + on_entry<_> / to_idle
            ,state<walk> + on_entry<_> / to_walk
            ,state<jump> + on_entry<_> / to_jump

            // 2. 地上での状態遷移
            ,state<idle> + event<OnInput> [is_walking] = state<walk>
            ,state<walk> + event<OnInput> [is_idle]    = state<idle>

            // 3. 【Any Stateによる一括遷移（サブステートの代替）】
            // state<_> は「現在の状態が何であれ」マッチします。
            // これにより、idleやwalkに個別にジャンプの遷移を書く必要がなくなります（条件爆発の回避）。
            ,sml::state<_> + event<OnInput> [is_starting_jump] = state<jump>

            // 4. 【着地時の動的ルーティング】
            // サブステートを持たないフラット構造なので、イベントの中身を評価して
            // 1フレームの遅延もなく直接正しい状態(walk または idle)へ遷移できます。
            ,state<jump> + event<OnInput> [!is_jumping && is_walking] = state<walk>
            ,state<jump> + event<OnInput> [!is_jumping && is_idle]    = state<idle>
        );
    }
};

} // namespace tr1

// ==========================================
// GoogleTest スイート
// ==========================================
TEST(AnyStateTest, FlatDesignWithAnyState) {
    Context ctx;
    sml::sm<tr1::MainSm> player_sm{ctx};

    // 1. 初期化直後
    EXPECT_EQ(ctx.state, MovementState::Idle);

    // 2. 歩き出し (idle -> walk)
    player_sm.process_event(OnInput{0.5f, false});
    EXPECT_EQ(ctx.state, MovementState::Walk);

    // 3. ジャンプ (walk -> state<_> によって jump)
    player_sm.process_event(OnInput{0.5f, true});
    EXPECT_EQ(ctx.state, MovementState::Jump);

    // 4. 【テスト成功】着地して「そのまま歩き続ける」(jump -> walk)
    // フラット構造の直接遷移により、無名遷移や2回呼び出しを使わずに即座にWalkへ戻ります。
    player_sm.process_event(OnInput{0.5f, false});
    EXPECT_EQ(ctx.state, MovementState::Walk);

    // 5. 再度ジャンプ (walk -> state<_> によって jump)
    player_sm.process_event(OnInput{0.5f, true});
    EXPECT_EQ(ctx.state, MovementState::Jump);

    // 6. 着地して「立ち止まる」(jump -> idle)
    player_sm.process_event(OnInput{0.0f, false});
    EXPECT_EQ(ctx.state, MovementState::Idle);
    
    // 7. 止まった状態からのジャンプ (idle -> state<_> によって jump)
    player_sm.process_event(OnInput{0.0f, true});
    EXPECT_EQ(ctx.state, MovementState::Jump);
}
} // namespace sml_any_state
