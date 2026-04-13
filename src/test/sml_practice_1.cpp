// sml_test_base.cpp
#include <gtest/gtest.h>
#include <boost/sml.hpp>

namespace sml_test_base {
namespace sml = boost::sml;

#if 0
TEST(SmlTest, test1)
{
	using sml::literals::operator""_s;	

	sml::sm<character_movement> player_sm;
	player_sm.process_event(on_update{0.0f}); // 何も起こらない (すでにIDLE)

	ASSERT_TRUE(player_sm.is("idle"_s));
}
#endif

} // namespace sml_test_base
