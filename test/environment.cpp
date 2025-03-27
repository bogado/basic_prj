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
    REQUIRE(!test.has_value());
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

TEST_CASE("enviroment_defintions", "[environment,definition]")
{
    env::environment env_test{};
    auto env = env_test.getEnv();

    REQUIRE(env != nullptr);
    REQUIRE(*env == nullptr); 

    env_test.set("USER") = "me";
    REQUIRE(env_test.import("HOME"));
    env = env_test.getEnv();

    REQUIRE(env[0] != nullptr);
    REQUIRE(env[1] != nullptr);
    REQUIRE(env[2] == nullptr);
}

}
