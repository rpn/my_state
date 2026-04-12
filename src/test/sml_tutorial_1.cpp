// sml_tutorial_1.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_tutorial_1 {
namespace sml = boost::sml;

// ------------------------------------------------------------
// Tutorial 1: 最小構成の状態遷移
// 目的:
// - 初期状態の書き方
// - event を投げて状態が変わる流れ
// ------------------------------------------------------------
struct EvStart {};
struct EvStop {};

enum class BasicState {
    Idle,
    Running,
};

struct BasicContext {
    BasicState state = BasicState::Idle;
};

struct BasicMachine {
    auto operator()() const {
        using namespace sml;

        auto on_idle = [](BasicContext& ctx) { ctx.state = BasicState::Idle; };
        auto on_running = [](BasicContext& ctx) { ctx.state = BasicState::Running; };

        return make_transition_table(
            // `*` を付けた状態が初期状態になる。
            // 文字列リテラルではなく型状態(class Idle/Running)を使う。
            *state<class Idle> + on_entry<_> / on_idle,
             state<class Running> + on_entry<_> / on_running,
             state<class Idle> + event<EvStart> = state<class Running>,
             state<class Running> + event<EvStop> = state<class Idle>
        );
    }
};

TEST(SmlTutorial1, basic_transition)
{
    BasicContext ctx;
    // 依存オブジェクトとして ctx を注入し、状態を enum で観測する。
    sml::sm<BasicMachine> sm{ctx};
    // 状態マシン生成直後は初期状態 "idle"。
    ASSERT_EQ(BasicState::Idle, ctx.state);

    // EvStart を送ると idle -> running へ遷移する。
    sm.process_event(EvStart{});
    ASSERT_EQ(BasicState::Running, ctx.state);

    // EvStop で running -> idle に戻る。
    sm.process_event(EvStop{});
    ASSERT_EQ(BasicState::Idle, ctx.state);
}

// ------------------------------------------------------------
// Tutorial 2: ガード条件とフォールバック遷移
// 目的:
// - 同じイベントに対する複数遷移の優先順
// - 遷移しないが副作用だけ発生するケース
// ------------------------------------------------------------
struct EvOpen {};
struct EvClose {};

struct DoorContext {
    enum class State {
        Closed,
        Open,
    };

    State state = State::Closed;
    bool has_key = false;
    int denied_count = 0;
};

struct GuardedDoorMachine {
    auto operator()() const {
        using namespace sml;

        auto can_open = [](const DoorContext& ctx) {
            return ctx.has_key;
        };

        auto count_denied = [](DoorContext& ctx) {
            ++ctx.denied_count;
        };

        auto on_closed = [](DoorContext& ctx) {
            ctx.state = DoorContext::State::Closed;
        };

        auto on_open = [](DoorContext& ctx) {
            ctx.state = DoorContext::State::Open;
        };

        return make_transition_table(
            *state<class Closed> + on_entry<_> / on_closed,
             state<class Open> + on_entry<_> / on_open,
            // 同じ EvOpen でも上から順に評価される。
            // has_key == true のときだけ open へ遷移。
             state<class Closed> + event<EvOpen> [can_open] = state<class Open>,
            // ガードに通らなければこの行が処理される。
            // 状態は closed のまま、拒否回数だけ増やす。
             state<class Closed> + event<EvOpen> / count_denied,
             state<class Open> + event<EvClose> = state<class Closed>
        );
    }
};

TEST(SmlTutorial1, guard_and_fallback)
{
    DoorContext ctx;
    sml::sm<GuardedDoorMachine> sm{ctx};

    // 初期は鍵なしで closed。
    ASSERT_EQ(DoorContext::State::Closed, ctx.state);
    ASSERT_EQ(0, ctx.denied_count);

    // 鍵がないので開かない。代わりに denied_count が増える。
    sm.process_event(EvOpen{});
    ASSERT_EQ(DoorContext::State::Closed, ctx.state);
    ASSERT_EQ(1, ctx.denied_count);

    // 鍵を持ったので、次の EvOpen で open に遷移できる。
    ctx.has_key = true;
    sm.process_event(EvOpen{});
    ASSERT_EQ(DoorContext::State::Open, ctx.state);
    ASSERT_EQ(1, ctx.denied_count);

    // EvClose で closed に戻る。
    sm.process_event(EvClose{});
    ASSERT_EQ(DoorContext::State::Closed, ctx.state);
}

// ------------------------------------------------------------
// Tutorial 3: イベントのペイロードとアクション
// 目的:
// - イベントにデータを持たせる
// - ガードとアクションで状態とコンテキストを同時に更新する
// ------------------------------------------------------------
struct EvDamage {
    int amount = 0;
};

struct BattleContext {
    enum class State {
        Alive,
        Dead,
    };

    State state = State::Alive;
    int hp = 100;
};

struct BattleMachine {
    auto operator()() const {
        using namespace sml;

        auto will_die = [](const EvDamage& ev, const BattleContext& ctx) {
            return (ctx.hp - ev.amount) <= 0;
        };

        auto apply_damage = [](const EvDamage& ev, BattleContext& ctx) {
            ctx.hp -= ev.amount;
            if (ctx.hp < 0)
                ctx.hp = 0;
        };

        auto on_alive = [](BattleContext& ctx) { ctx.state = BattleContext::State::Alive; };
        auto on_dead = [](BattleContext& ctx) { ctx.state = BattleContext::State::Dead; };

        return make_transition_table(
            *state<class Alive> + on_entry<_> / on_alive,
             state<class Dead> + on_entry<_> / on_dead,
            // 先に「致死ダメージ」の分岐を書く。
            // 条件に当てはまれば alive -> dead へ遷移する。
             state<class Alive> + event<EvDamage> [will_die] / apply_damage = state<class Dead>,
            // 致死でなければ同じ alive に留まりつつ HP だけ減らす。
             state<class Alive> + event<EvDamage>            / apply_damage,
            // dead で追加ダメージを受けても dead のまま。
             state<class Dead> + event<EvDamage>             = state<class Dead>
        );
    }
};

TEST(SmlTutorial1, event_payload_and_action)
{
    BattleContext ctx;
    sml::sm<BattleMachine> sm{ctx};

    // 初期値確認。
    ASSERT_EQ(BattleContext::State::Alive, ctx.state);
    ASSERT_EQ(100, ctx.hp);

    // 40 ダメージは致死ではないので alive 維持。
    sm.process_event(EvDamage{40});
    ASSERT_EQ(BattleContext::State::Alive, ctx.state);
    ASSERT_EQ(60, ctx.hp);

    // 80 ダメージで 0 以下になるため dead へ遷移。
    sm.process_event(EvDamage{80});
    ASSERT_EQ(BattleContext::State::Dead, ctx.state);
    ASSERT_EQ(0, ctx.hp);
}

// ------------------------------------------------------------
// Tutorial 4: on_entry/on_exit と内部処理
// 目的:
// - 状態に入る/出るタイミングでフックする
// - 状態遷移なしの内部処理を表現する
// ------------------------------------------------------------
struct EvTick {};
struct EvEnemySpotted {};
struct EvEnemyLost {};

struct LifecycleContext {
    enum class State {
        Patrol,
        Combat,
    };

    State state = State::Patrol;
    int patrol_entry = 0;
    int patrol_exit = 0;
    int combat_entry = 0;
    int tick_count = 0;
};

struct LifecycleMachine {
    auto operator()() const {
        using namespace sml;

        auto on_patrol_entry = [](LifecycleContext& ctx) {
            ++ctx.patrol_entry;
            ctx.state = LifecycleContext::State::Patrol;
        };
        auto on_patrol_exit = [](LifecycleContext& ctx) { ++ctx.patrol_exit; };
        auto on_combat_entry = [](LifecycleContext& ctx) {
            ++ctx.combat_entry;
            ctx.state = LifecycleContext::State::Combat;
        };
        auto on_tick = [](LifecycleContext& ctx) { ++ctx.tick_count; };

        return make_transition_table(
              // patrol に入るたび entry カウンタを増やす。
            *state<class Patrol> + on_entry<_> / on_patrol_entry,
              // patrol から出るときに exit カウンタを増やす。
             state<class Patrol> + on_exit<_> / on_patrol_exit,
              // combat に入った回数を記録する。
             state<class Combat> + on_entry<_> / on_combat_entry,

               // 内部処理: patrol のまま tick_count を増やす。
             state<class Patrol> + event<EvTick> / on_tick,

             state<class Patrol> + event<EvEnemySpotted> = state<class Combat>,
             state<class Combat> + event<EvEnemyLost> = state<class Patrol>
        );
    }
};

TEST(SmlTutorial1, entry_exit_and_internal_processing)
{
    LifecycleContext ctx;
    sml::sm<LifecycleMachine> sm{ctx};

    // 生成時に patrol へ entry している。
    ASSERT_EQ(LifecycleContext::State::Patrol, ctx.state);
    ASSERT_EQ(1, ctx.patrol_entry);
    ASSERT_EQ(0, ctx.patrol_exit);

    // EvTick は内部処理なので状態は変わらない。
    sm.process_event(EvTick{});
    ASSERT_EQ(LifecycleContext::State::Patrol, ctx.state);
    ASSERT_EQ(1, ctx.patrol_entry);
    ASSERT_EQ(1, ctx.tick_count);

    // 敵発見で combat へ。patrol の exit と combat の entry が走る。
    sm.process_event(EvEnemySpotted{});
    ASSERT_EQ(LifecycleContext::State::Combat, ctx.state);
    ASSERT_EQ(1, ctx.patrol_exit);
    ASSERT_EQ(1, ctx.combat_entry);

    // 敵を見失うと patrol に戻り、patrol_entry が再度増える。
    sm.process_event(EvEnemyLost{});
    ASSERT_EQ(LifecycleContext::State::Patrol, ctx.state);
    ASSERT_EQ(2, ctx.patrol_entry);
}

// ------------------------------------------------------------
// Tutorial 5: サブステートマシン(複合状態)
// 目的:
// - 親状態と子状態を同時に管理する
// - 親の遷移で子マシン全体を出入りする
// ------------------------------------------------------------
struct EvInRange {};
struct EvOutOfRange {};

enum class TopState {
    Patrol,
    Combat,
};

enum class CombatSubState {
    None,
    Chase,
    Attack,
};

struct HierarchyContext {
    TopState top = TopState::Patrol;
    CombatSubState sub = CombatSubState::None;
};

struct CombatSubMachine {
    auto operator()() const {
        using namespace sml;

        auto to_chase = [](HierarchyContext& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
        };

        auto to_attack = [](HierarchyContext& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Attack;
        };

        return make_transition_table(
            // CombatSubMachine に入った直後の子初期状態は Chase。
            *state<class Chase> + on_entry<_> / to_chase,
            // 射程内なら Attack へ。
             state<class Chase> + event<EvInRange> / to_attack = state<class Attack>,
            // 射程外なら Chase へ戻る。
             state<class Attack> + event<EvOutOfRange> / to_chase = state<class Chase>
        );
    }
};

struct RootMachine {
    auto operator()() const {
        using namespace sml;

        auto to_patrol = [](HierarchyContext& ctx) {
            ctx.top = TopState::Patrol;
            ctx.sub = CombatSubState::None;
        };

        auto to_combat = [](HierarchyContext& ctx) {
            ctx.top = TopState::Combat;
            ctx.sub = CombatSubState::Chase;
        };

        return make_transition_table(
            // ルート初期状態は Patrol。
            *state<class Patrol> + on_entry<_> / to_patrol,
            // 敵発見で CombatSubMachine に入る。
             state<class Patrol> + event<EvEnemySpotted> / to_combat = state<CombatSubMachine>,
            // 敵を見失えば親状態 Patrol に戻る。
             state<CombatSubMachine> + event<EvEnemyLost> / to_patrol = state<class Patrol>
        );
    }
};

TEST(SmlTutorial1, hierarchical_state_machine)
{
    HierarchyContext ctx;
    sml::sm<RootMachine> sm{ctx};

    // 初期は Patrol / None。
    ASSERT_EQ(TopState::Patrol, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);

    // 親: Combat、子: Chase に入る。
    sm.process_event(EvEnemySpotted{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Chase, ctx.sub);

    // 子状態だけ Attack に遷移。
    sm.process_event(EvInRange{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Attack, ctx.sub);

    // 子状態だけ Chase に戻る。
    sm.process_event(EvOutOfRange{});
    ASSERT_EQ(TopState::Combat, ctx.top);
    ASSERT_EQ(CombatSubState::Chase, ctx.sub);

    // 親ごと Patrol に戻り、子は None にリセット。
    sm.process_event(EvEnemyLost{});
    ASSERT_EQ(TopState::Patrol, ctx.top);
    ASSERT_EQ(CombatSubState::None, ctx.sub);
}

} // namespace sml_tutorial_1
