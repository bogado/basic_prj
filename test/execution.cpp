// main.cpp                                                                        -*-C++-*-
#include "util/execution.hpp"

#include "util/system.hpp"
#include <catch2/catch_all.hpp>

#include <source_location>
#include <string_view>

static constexpr auto test_dir = []() {
    auto file = std::string_view{ std::source_location::current().file_name() };
    auto dir  = file.substr(0, file.find_last_of('/'));
    return dir;
}();

static constexpr auto test_data = []() {
    static constexpr decltype(auto) data_dir = "/test_data\0";
    static constexpr auto           dir      = []() {
        std::array<char, std::size(test_dir) + sizeof(data_dir)> data{};
        auto out = std::ranges::copy(test_dir, std::ranges::begin(data)).out;
        std::ranges::copy(data_dir, out);
        return data;
    }();
    return std::string_view{ dir.data() };
}();

using namespace std::literals;

static constexpr auto expected = std::array{ "123\n"sv, "abc\n"sv, "line_data.hpp\n"sv };

using namespace std::literals;

TEST_CASE("Execution of external command", "[execute][pipe][buffer][generator]")
{
    auto handler = vb::execution(vb::io_set::OUT);
    handler.execute(vb::fs::path("/bin/ls"sv), std::array{ vb::fs::path{ test_data } });
    auto expectation = expected.begin();
    for (auto line : handler.lines<vb::std_io::OUT>()) {
        if (line == "") {
            continue;
        }
        REQUIRE(expectation != expected.end());
        CHECK(*expectation == line);
        ++expectation;
    }
    CHECK(std::distance(expectation, expected.end()) == 0);

    REQUIRE(handler.wait() == 0);
}

TEST_CASE("Execution ranges read", "[execute][pipe][ranges][generator]")
{
    auto handler = vb::execution(vb::io_set::OUT);
    handler.execute(vb::fs::path("/bin/ls"sv), std::array{ vb::fs::path{ test_data } });
    auto lines = std::vector<std::string>{};

    std::ranges::copy(handler.lines<vb::std_io::OUT>(), std::back_inserter(lines));
    REQUIRE(handler.wait() == 0);
}

TEST_CASE("Execution with vector", "[execute][pipe][buffer][generator]")
{
    auto                     handler = vb::execution();
    std::vector<std::string> args{ "hello", "world" };
    handler.execute("echo"s, args);
    REQUIRE(handler.wait() == 0);
}

TEST_CASE("Execution of external command with environment", "[execute][pipe][buffer][generator][environment]")
{
    auto handler     = vb::execution(vb::io_set::OUT);
    auto environment = vb::env::environment{};
    environment.import("PATH");
    environment.import("HOME");
    environment.import("USER");
    environment.set("A") = "a";
    handler.execute(vb::fs::path{ "/bin/bash" }, std::array{ "-c"s, "echo $A"s }, environment);
    std::string result;
    for (auto line : handler.lines<vb::std_io::OUT>()) {
        result += line;
    }
    REQUIRE(result == "a\n");
}
