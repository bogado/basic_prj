#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include "util/environment.hpp"

namespace vb::testing {

TEST_CASE("variable", "[variable]")
{
    using namespace vb::literals;
    env::variable test{"test"};
    REQUIRE(test.name() == "test"_env);
    REQUIRE(test.value<std::string>() == ""sv);
    REQUIRE(test.value_str() == ""sv);
    test.set("value");
    REQUIRE(test.value_str() == "value");
}

TEST_CASE("environment_test", "[environment]")
{
    env::environment env_test{};
    REQUIRE(env_test.size() == 0);
    env_test.set("test") = "value";
    REQUIRE(env_test.size() == 1);

    env_test.set("check") = 32;
    REQUIRE(env_test.size() == 2);

    REQUIRE(env_test.get<std::string>("test") == "value");
    REQUIRE(env_test.get<std::string>("check") == "32");
    REQUIRE(env_test.get<int>("check") == 32);
}

}
