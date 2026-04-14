// sml_p3.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_p3 {
namespace sml = boost::sml;

enum class MyState {
	Foo,
	Bar,
	Baz,
};

struct Context {
	MyState state {};
};

namespace st {
struct Foo {};
struct Bar {};
struct Baz {};
} // namespace st


namespace tr1 {
struct MySm {
	auto operator()() const {
		using namespace sml;

		auto to_foo = [](Context& ctx) -> void
		{
			ctx.state = MyState::Foo;
		};

		auto to_bar = [](Context& ctx) -> void
		{
			ctx.state = MyState::Bar;
		};

		auto to_baz = [](Context& ctx) -> void
		{
			ctx.state = MyState::Baz;
		};

		return make_transition_table(
			*state<st::Foo> + on_entry<_> / to_foo
			,state<st::Bar> + on_entry<_> / to_bar
			,state<st::Baz> + on_entry<_> / to_baz

			,state<st::Foo> + event<st::Bar> = state<st::Bar>
			,state<st::Bar> + event<st::Baz> = state<st::Baz>
			,state<st::Baz> + event<st::Foo> = state<st::Foo>
		);
	}
};


struct MyLogger {
	bool short_name = true;

	// ① イベントが処理されようとした時に呼ばれる
	template <class SM, class TEvent>
	void log_process_event(const TEvent&) {
		printf("[process_event] %s\n", 
			sml::aux::get_type_name<TEvent>());
	}

	// ② ガード条件が評価された結果をログに出す
	template <class SM, class TGuard, class TEvent>
	void log_guard(const TGuard&, const TEvent&, bool result) {
		printf("[guard] %s %s %s\n", 
			sml::aux::get_type_name<TGuard>(),
			sml::aux::get_type_name<TEvent>(), 
			(result ? "[OK]" : "[Reject]"));
	}

	// ③ アクションが実行された時に呼ばれる
	template <class SM, class TAction, class TEvent>
	void log_action(const TAction&, const TEvent&) {
		printf("[action] %s %s\n", 
			sml::aux::get_type_name<TAction>(),
			sml::aux::get_type_name<TEvent>());
	}

	// ④ 状態が実際に遷移した時に呼ばれる
	template <class SM, class TSrcState, class TDstState>
	void log_state_change(const TSrcState& src, const TDstState& dst) {
		// src と dst には状態名を文字列として取得できる c_str() が用意されています
		printf("[transition] %s -> %s\n", 
			src.c_str(), 
			dst.c_str());
	}
};

TEST(SmlP3, test1)
{
	MyLogger logger;
	Context ctx;
	sml::sm<MySm, sml::logger<MyLogger>> sm{ctx, logger};
	ASSERT_EQ(MyState::Foo, ctx.state);

	sm.process_event(st::Bar{});
	ASSERT_EQ(MyState::Bar, ctx.state);

	sm.process_event(st::Baz{});
	ASSERT_EQ(MyState::Baz, ctx.state);

	sm.process_event(st::Foo{});
	ASSERT_EQ(MyState::Foo, ctx.state);
}
} // namespace tr1

} // namespace sml_p3
