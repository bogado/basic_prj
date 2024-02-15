// main.cpp                                                                        -*-C++-*-
#include <catch2/catch_all.hpp>

#include "util/execution.hpp"

#include <string_view>
#include <source_location>

static constexpr auto test_dir = []() {
    auto file = std::string_view{std::source_location::current().file_name()};
    auto dir = file.substr(0, file.find_last_of('/'));
    return dir;
}();

using namespace std::literals;

static constexpr auto expected = std::array{
    "test_data\n"sv,
    "123\n"sv,
    "abc\n"sv };

using namespace std::literals;

TEST_CASE("Execution of external command", "[execute]")
{
    auto handler = vb::execution(vb::fs::path("/usr/bin/ls"sv), std::array{std::string{vb::fs::path(test_dir) / "test_data"} });
    auto expectation = expected.begin();
    for (auto line : handler.stdout_lines()) {
        CHECK(expectation != expected.end());
        CHECK(*expectation == line);
        ++expectation;
    }
    CHECK(std::distance(expectation, expected.end()) == 0);

    REQUIRE(handler.wait() == 0);
}
