
#include "util/options.hpp"

#include <catch2/catch_all.hpp>
namespace vb::opt {
using namespace std::literals;

TEST_CASE("Test options construction", "[options init]")
{
    constexpr auto option = basic_option<bool>("-t, --test Testing the options");
    REQUIRE(option.shortkey() == 't');
    REQUIRE(option == "-t");
    REQUIRE(option.key() == "test"sv);
}

TEST_CASE("parsing bool options", "[options parse bool]")
{
    {
        constexpr auto option = basic_option<bool>("-t, --test Testing the options");
        REQUIRE(option.parse("-g") == false);
        REQUIRE(option.parse("--not-test") == false);
        REQUIRE(option.parse("--test-not") == false);
        REQUIRE(option.parse("--test") == true);
        REQUIRE(option.parse("--Test") == false);
        REQUIRE(option.parse("-test") == true);
        REQUIRE(option.parse("-Test") == false);
        REQUIRE(option.parse("-T") == false);
        REQUIRE(option.parse("-t") == true);
    }
    {
        constexpr auto option = basic_option<bool>("-T, --test Testing the options");
        REQUIRE(option.parse("-g") == false);
        REQUIRE(option.parse("--not-test") == false);
        REQUIRE(option.parse("--test-not") == false);
        REQUIRE(option.parse("--test") == true);
        REQUIRE(option.parse("-test") == false);
        REQUIRE(option.parse("-Test") == true);
        REQUIRE(option.parse("-t") == false);
        REQUIRE(option.parse("-T") == true);
    }
}

TEST_CASE("parsing string options", "[options parse string]")
{
    constexpr auto option = basic_option<std::string>("-t, --test Testing the string options");
    REQUIRE(option.parse("-g=test").has_value() == false);
    REQUIRE(option.parse("--not-test=test").has_value() == false);
    REQUIRE(option.parse("--test-not=test").has_value() == false);
    REQUIRE(option.parse("--test").has_value() == false);
    REQUIRE(option.parse("--test=test").has_value() == true);
    REQUIRE(option.parse("--Test=test").has_value() == false);
    REQUIRE(option.parse("-ttest").has_value() == true);
    REQUIRE(option.parse("-Ttest").has_value() == false);
    REQUIRE(option.parse("-Ttest").has_value() == false);
    REQUIRE(option.parse("-ttest").has_value() == true);
}

TEST_CASE("parsing string option values", "[options parse string values]")
{
    constexpr auto option = basic_option<std::string>("-t, --test Testing the string options");
    REQUIRE(option.parse("--test=test").value() == "test"sv);
    REQUIRE(option.parse("-ttest").value() == "test"sv);
    REQUIRE(option.parse("-ttest").value() == "test"sv);
}

} // namespace vb::opt
