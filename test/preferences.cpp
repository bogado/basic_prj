#include <catch2/catch_all.hpp>

#include <string_view>

#include "util/preferences.hpp"

using namespace std::literals;

TEST_CASE("long option", "[prefference][long]")
{
    auto arg = vb::argument{"-l, --long : long description"};
    REQUIRE(arg.long_option == "long"sv);
}
