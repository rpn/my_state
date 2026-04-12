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

struct BasicMachine {
    auto operator()() const {
        using namespace sml;
        return make_transition_table(
            // `*` を付けた状態が初期状態になる。
            *"idle"_s + event<EvStart> = "running"_s,
             "running"_s + event<EvStop> = "idle"_s
        );
    }
};

TEST(SmlTutorial1, basic_transition)
{
    using sml::literals::operator""_s;

    // 状態マシン生成直後は初期状態 "idle"。
    sml::sm<BasicMachine> sm;
    ASSERT_TRUE(sm.is("idle"_s));

    // EvStart を送ると idle -> running へ遷移する。
    sm.process_event(EvStart{});
    ASSERT_TRUE(sm.is("running"_s));

    // EvStop で running -> idle に戻る。
    sm.process_event(EvStop{});
    ASSERT_TRUE(sm.is("idle"_s));
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

        return make_transition_table(
            // 同じ EvOpen でも上から順に評価される。
            // has_key == true のときだけ open へ遷移。
            *"closed"_s + event<EvOpen> [can_open] = "open"_s,
            // ガードに通らなければこの行が処理される。
            // 状態は closed のまま、拒否回数だけ増やす。
             "closed"_s + event<EvOpen> / count_denied,
             "open"_s + event<EvClose> = "closed"_s
        );
    }
};

TEST(SmlTutorial1, guard_and_fallback)
{
    using sml::literals::operator""_s;

    DoorContext ctx;
    sml::sm<GuardedDoorMachine> sm{ctx};

    // 初期は鍵なしで closed。
    ASSERT_TRUE(sm.is("closed"_s));
    ASSERT_EQ(0, ctx.denied_count);

    // 鍵がないので開かない。代わりに denied_count が増える。
    sm.process_event(EvOpen{});
    ASSERT_TRUE(sm.is("closed"_s));
    ASSERT_EQ(1, ctx.denied_count);

    // 鍵を持ったので、次の EvOpen で open に遷移できる。
    ctx.has_key = true;
    sm.process_event(EvOpen{});
    ASSERT_TRUE(sm.is("open"_s));
    ASSERT_EQ(1, ctx.denied_count);

    // EvClose で closed に戻る。
    sm.process_event(EvClose{});
    ASSERT_TRUE(sm.is("closed"_s));
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

        return make_transition_table(
            // 先に「致死ダメージ」の分岐を書く。
            // 条件に当てはまれば alive -> dead へ遷移する。
            *"alive"_s + event<EvDamage> [will_die] / apply_damage = "dead"_s,
            // 致死でなければ同じ alive に留まりつつ HP だけ減らす。
             "alive"_s + event<EvDamage>            / apply_damage,
            // dead で追加ダメージを受けても dead のまま。
             "dead"_s + event<EvDamage>             = "dead"_s
        );
    }
};

TEST(SmlTutorial1, event_payload_and_action)
{
    using sml::literals::operator""_s;

    BattleContext ctx;
    sml::sm<BattleMachine> sm{ctx};

    // 初期値確認。
    ASSERT_TRUE(sm.is("alive"_s));
    ASSERT_EQ(100, ctx.hp);

    // 40 ダメージは致死ではないので alive 維持。
    sm.process_event(EvDamage{40});
    ASSERT_TRUE(sm.is("alive"_s));
    ASSERT_EQ(60, ctx.hp);

    // 80 ダメージで 0 以下になるため dead へ遷移。
    sm.process_event(EvDamage{80});
    ASSERT_TRUE(sm.is("dead"_s));
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
    int patrol_entry = 0;
    int patrol_exit = 0;
    int combat_entry = 0;
    int tick_count = 0;
};

struct LifecycleMachine {
    auto operator()() const {
        using namespace sml;

        auto on_patrol_entry = [](LifecycleContext& ctx) { ++ctx.patrol_entry; };
        auto on_patrol_exit = [](LifecycleContext& ctx) { ++ctx.patrol_exit; };
        auto on_combat_entry = [](LifecycleContext& ctx) { ++ctx.combat_entry; };
        auto on_tick = [](LifecycleContext& ctx) { ++ctx.tick_count; };

        return make_transition_table(
              // patrol に入るたび entry カウンタを増やす。
            *"patrol"_s + on_entry<_> / on_patrol_entry,
              // patrol から出るときに exit カウンタを増やす。
             "patrol"_s + on_exit<_> / on_patrol_exit,
              // combat に入った回数を記録する。
             "combat"_s + on_entry<_> / on_combat_entry,

               // 内部処理: patrol のまま tick_count を増やす。
             "patrol"_s + event<EvTick> / on_tick,

             "patrol"_s + event<EvEnemySpotted> = "combat"_s,
             "combat"_s + event<EvEnemyLost> = "patrol"_s
        );
    }
};

TEST(SmlTutorial1, entry_exit_and_internal_processing)
{
    using sml::literals::operator""_s;

    LifecycleContext ctx;
    sml::sm<LifecycleMachine> sm{ctx};

    // 生成時に patrol へ entry している。
    ASSERT_TRUE(sm.is("patrol"_s));
    ASSERT_EQ(1, ctx.patrol_entry);
    ASSERT_EQ(0, ctx.patrol_exit);

    // EvTick は内部処理なので状態は変わらない。
    sm.process_event(EvTick{});
    ASSERT_TRUE(sm.is("patrol"_s));
    ASSERT_EQ(1, ctx.patrol_entry);
    ASSERT_EQ(1, ctx.tick_count);

    // 敵発見で combat へ。patrol の exit と combat の entry が走る。
    sm.process_event(EvEnemySpotted{});
    ASSERT_TRUE(sm.is("combat"_s));
    ASSERT_EQ(1, ctx.patrol_exit);
    ASSERT_EQ(1, ctx.combat_entry);

    // 敵を見失うと patrol に戻り、patrol_entry が再度増える。
    sm.process_event(EvEnemyLost{});
    ASSERT_TRUE(sm.is("patrol"_s));
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
