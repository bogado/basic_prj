#include "util/preferences.hpp"

#include <catch2/catch_all.hpp>

#include <string_view>

using namespace std::literals;

TEST_CASE("long option", "[prefference][long]")
{
    auto arg = vb::argument{ "-l, --long : long description" };
    REQUIRE(arg.long_option == "long"sv);
}
