#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "util/environment.hpp"

namespace vb::testing {

TEST_CASE("variable_name", "[variable_name]")
{
    using namespace vb::literals;
    env::variable_name test = "Test123"_env;
    REQUIRE(test.name() == "Test123");
    REQUIRE(!test.value_from_system().has_value());
    REQUIRE(test.value_or("hi") == "hi");
}

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
    REQUIRE(!env_test.get("test").has_value());
    REQUIRE(!env_test.value_for("test").has_value());

    env_test.set("test") = "value";
    REQUIRE(env_test.size() == 1);
    REQUIRE(env_test.value_for("test").has_value());
    REQUIRE(env_test.value_for("test").value() == "value");

    env_test.set("check") = 32;
    REQUIRE(env_test.size() == 2);

    REQUIRE(env_test.get("test").has_value());
    REQUIRE(env_test.get("test").value().value_str() == "value");
    REQUIRE(env_test.get("check").has_value());
    REQUIRE(env_test.get("check").value().value_str() == "32");
}

TEST_CASE("enviroment_defintions", "[environment,definition]")
{
    env::environment env_test{};
    auto env = env_test.getEnv();

    REQUIRE(env.empty());

    env_test.set("USER") = "me";
    REQUIRE(env_test.import("HOME"));
    env = env_test.getEnv();

    REQUIRE(env.size() == 2);
    REQUIRE_THAT(env, Catch::Matchers::VectorContains("USER=me"s));
}

}
