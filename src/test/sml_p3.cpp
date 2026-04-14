// sml_p3.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>
#include <fmt/printf.h>

#include "my_logger.h"
#include "sml_dump.h"

namespace sml_p3 {
namespace sml = boost::sml;
using namespace sml_util;

enum class MyState {
	Foo,
	Bar,
	Baz,
	Quax,
};

struct Context {
	MyState state {};
};

namespace st {
struct Foo {};
struct Bar {};
struct Baz {};
struct Quax {};
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

TEST(SmlP3, test1)
{
	std::vector<std::string> buf;
	MyLogger logger{&buf};
	Context ctx;
	sml::sm<MySm, sml::logger<MyLogger>> sm{ctx, logger};

	ASSERT_EQ(MyState::Foo, ctx.state);
	ASSERT_EQ(
		"[process_event] initial>\n"
		"[action] <lambda_1> initial>", extract(buf));

	sm.process_event(st::Bar{});
	ASSERT_EQ(MyState::Bar, ctx.state);
	ASSERT_EQ(
		"[process_event] Bar\n"
		"[transition] Foo -> Bar\n"
		"[process_event] Bar>\n"
		"[action] <lambda_2> Bar>", extract(buf));

	sm.process_event(st::Baz{});
	ASSERT_EQ(MyState::Baz, ctx.state);
	ASSERT_EQ(
		"[process_event] Baz\n"
		"[transition] Bar -> Baz\n"
		"[process_event] Baz>\n"
		"[action] <lambda_3> Baz>", extract(buf));

	sm.process_event(st::Foo{});
	ASSERT_EQ(MyState::Foo, ctx.state);
	ASSERT_EQ(
		"[process_event] Foo\n"
		"[transition] Baz -> Foo\n"
		"[process_event] Foo>\n"
		"[action] <lambda_1> Foo>", extract(buf));

	dump(sm);
}

struct MySm2 {
	auto operator()() const {
		using namespace sml;

		auto to_quax = [](Context& ctx) -> void
		{
			ctx.state = MyState::Quax;
		};

		return make_transition_table(
			*state<MySm> + event<st::Quax> = state<st::Quax>
			,state<st::Quax> + on_entry<_> / to_quax

			,state<st::Quax> + event<st::Foo> = state<MySm>
		);
	}
};

TEST(SmlP3, test2)
{
	std::vector<std::string> buf;
	MyLogger logger{&buf};
	Context ctx;
	sml::sm<MySm2, sml::logger<MyLogger>> sm{ctx, logger};

	ASSERT_EQ(
		"[process_event] initial>\n"
		"[process_event] initial>\n"
		"[action] <lambda_1> initial>", extract(buf));

	ASSERT_EQ(MyState::Foo, ctx.state);

	sm.process_event(st::Bar{});
	ASSERT_EQ(MyState::Bar, ctx.state);
	ASSERT_EQ(
		"[process_event] Bar\n"
		"[process_event] Bar\n"
		"[transition] Foo -> Bar\n"
		"[process_event] Bar>\n"
		"[action] <lambda_2> Bar>", extract(buf));

	sm.process_event(st::Baz{});
	ASSERT_EQ(MyState::Baz, ctx.state);
	ASSERT_EQ(
		"[process_event] Baz\n"
		"[process_event] Baz\n"
		"[transition] Bar -> Baz\n"
		"[process_event] Baz>\n"
		"[action] <lambda_3> Baz>", extract(buf));

	sm.process_event(st::Quax{});
	ASSERT_EQ(MyState::Quax, ctx.state);
	ASSERT_EQ(
		"[process_event] Quax\n"
		"[transition] MySm -> Quax\n"
		"[process_event] Quax>\n"
		"[action] <lambda_1> Quax>", extract(buf));

	sm.process_event(st::Foo{});
	ASSERT_EQ(MyState::Foo, ctx.state);
	ASSERT_EQ(
		"[process_event] Foo\n"
		"[transition] Quax -> MySm\n"
		"[process_event] Foo>\n"
		"[process_event] Foo>\n"
		"[action] <lambda_1> Foo>", extract(buf));

	dump_with_composite<MySm>(sm);
}


} // namespace tr1

} // namespace sml_p3
