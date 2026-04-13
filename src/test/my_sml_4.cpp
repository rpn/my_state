// my_sml_4.cpp
#if 0
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace my_sml_4 {
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

auto is_idle = [](const OnInput& e)
{
	return e.input_power == 0.0f;
};

auto is_walking = [](const OnInput& e)
{
	return e.input_power > 0.0f && e.input_power < 1.0f;
};

auto is_jumping = [](const OnInput& e)
{
	return e.jump;
};

namespace tr1 {
struct GroundSm {
	auto operator()() const {
		using namespace sml;

		auto to_idle = [](Context& ctx)
		{
			ctx.state = MovementState::Idle;
		};
		auto to_walk = [](Context& ctx)
		{
			ctx.state = MovementState::Walk;
		};
		auto to_jump = [](Context& ctx)
		{
			ctx.state = MovementState::Jump;
		};
		
		return make_transition_table(
			*"idle"_s + on_entry<_> / to_idle
			,"walk"_s + on_entry<_> / to_walk
			,"jump"_s + on_entry<_> / to_jump

			,"jump"_s + event<OnInput> [!is_jumping && is_idle] = "idle"_s
			,"jump"_s + event<OnInput> [!is_jumping && is_walking] = "walk"_s

			,"idle"_s + event<OnInput> [is_jumping] = "jump"_s
			,"idle"_s + event<OnInput> [is_walking] = "walk"_s

			,"walk"_s + event<OnInput> [is_jumping] = "jump"_s
			,"walk"_s + event<OnInput> [is_idle] = "idle"_s
		);
	}
};

TEST(SmlTest4, test1)
{
	Context ctx;
	sml::sm<GroundSm> sm{ctx};
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);

	sm.process_event(OnInput{0.0f});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.0f, true});
	ASSERT_EQ(MovementState::Jump, ctx.state);

	sm.process_event(OnInput{0.0f, false});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f, true});
	ASSERT_EQ(MovementState::Jump, ctx.state);

	sm.process_event(OnInput{0.1f, false});
	ASSERT_EQ(MovementState::Walk, ctx.state);

	sm.process_event(OnInput{0.1f, true});
	ASSERT_EQ(MovementState::Jump, ctx.state);
}
} // namespace tr1

#if 0
namespace tr2 {

struct GroundSm {
	auto operator()() const {
		using namespace sml;

		auto to_idle = [](Context& ctx)
		{
			ctx.state = MovementState::Idle;
		};
		auto to_walk = [](Context& ctx)
		{
			ctx.state = MovementState::Walk;
		};
		
		return make_transition_table(
			*"idle"_s + on_entry<_> / to_idle
			,"walk"_s + on_entry<_> / to_walk

			,"idle"_s + event<OnInput> [is_walking] = "walk"_s
			,"walk"_s + event<OnInput> [is_idle] = "idle"_s
		);

	}
};

struct MovementSm {
	auto operator()() const {
		using namespace sml;

		auto to_jump = [](Context& ctx)
		{
			ctx.state = MovementState::Jump;
		};

		return make_transition_table(
			//*state<GroundSm>
			*"jump"_s + on_entry<_> / to_jump

			,state<GroundSm> + event<OnInput> [is_jumping] = "jump"_s

			,"jump"_s + event<OnInput> [!is_jumping] = state<GroundSm>
		);
	}
};

TEST(SmlTest4, test2)
{
	Context ctx;
	sml::sm<MovementSm> sm{ctx};
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.1f});
	ASSERT_EQ(MovementState::Walk, ctx.state);

	sm.process_event(OnInput{0.0f});
	ASSERT_EQ(MovementState::Idle, ctx.state);

	sm.process_event(OnInput{0.0f, true});
	ASSERT_EQ(MovementState::Jump, ctx.state);
}

} // namespace tr2
#endif

namespace tr3 {

// 独自のロガー構造体を定義
struct MyLogger {
	template <class SM, class TEvent>
	void log_process_event(const TEvent&) {
		printf("[SML] イベント処理開始\n");
	}

	template <class SM, class TGuard, class TEvent>
	void log_guard(const TGuard&, const TEvent&, bool result) {
		printf("[SML] ガード判定: %s\n", result ? "OK (true)" : "NG (false)");
	}

	template <class SM, class TAction, class TEvent>
	void log_action(const TAction&, const TEvent&) {
		printf("[SML] アクション実行\n");
	}

	template <class SM, class TSrcState, class TDstState>
	void log_state_change(const TSrcState&, const TDstState&) {
		printf("[SML] 状態遷移: 完了\n");
	}
};




struct idle {};
struct walk {};
struct jump {};

// 1. サブステート（地上での移動）の定義
struct GroundMovementSm {
	auto operator()() const {
		using namespace sml;

		auto to_idle = [](Context& ctx) -> void { ctx.state = MovementState::Idle; };
		auto to_walk = [](Context& ctx) -> void { ctx.state = MovementState::Walk; };
		
		return make_transition_table(
			// サブステートに入った際の初期状態は idle
			*state<idle> + on_entry<_> / to_idle
			,state<walk> + on_entry<_> / to_walk

			// 地上での状態遷移（ジャンプのことは一切気にしない）
			,state<idle> + event<OnInput> [is_walking] = state<walk>
			,state<walk> + event<OnInput> [is_idle]    = state<idle>
		);
	}
};

// 2. メインのステートマシン（全体管理）の定義
struct MainSm {
	auto operator()() const {
		using namespace sml;

		auto to_jump = [](Context& ctx) -> void
		{
			ctx.state = MovementState::Jump;
		};

		return make_transition_table(
			// システムの初期状態を「地上のサブステート」に設定する
			*state<GroundMovementSm> + event<OnInput> [is_jumping] = state<jump>			

			,state<jump> + on_entry<_> / to_jump

			// ジャンプ終了後、地上のサブステートへ戻る
			,state<jump> + event<OnInput> [!is_jumping] = state<GroundMovementSm>
		);
	}
};

TEST(SmlTest4, test3)
{
	using namespace sml;
	{
		MyLogger logger;
		Context ctx;
		sml::sm<GroundMovementSm, sml::logger<MyLogger>> sm{ctx, logger};
		ASSERT_EQ(MovementState::Idle, ctx.state);

		//sm.process_event(OnInput{0.0f, false});
		//ASSERT_EQ(MovementState::Idle, ctx.state);

		sm.process_event(OnInput{0.0f, true});
		bool xxx = sm.is(sml::state<tr3::jump>);
		ASSERT_TRUE(xxx);

		ASSERT_EQ(MovementState::Idle, ctx.state);
	}

	//{
	//	sml::sm<MainSm> sm{ctx};

	//	sm.process_event(OnInput{0.0f, false});
	//	ASSERT_EQ(MovementState::Walk, ctx.state);


	//	sm.process_event(OnInput{0.0f, true});
	//	ASSERT_EQ(MovementState::Walk, ctx.state);
	//}
}

} // namespace tr3
} // namespace my_sml_3
#endif

