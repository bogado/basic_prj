// main.cpp                                                                        -*-C++-*-
#include "util/system.hpp"
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
    "123\n"sv,
    "abc\n"sv,
    "line_data.hpp\n"sv
};

using namespace std::literals;

TEST_CASE("Execution of external command", "[execute][pipe][buffer][generator]")
{
    auto handler = vb::execution(vb::io_set::OUT);
    handler.execute(vb::fs::path("/bin/ls"sv), std::array{(vb::fs::path(test_dir) / "test_data").string()});
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

#if 0 // This test is not yet working as expected
TEST_CASE("Execution of a external command that reads the stdin", "[execute][pipe][buffer][generator]")
{
    auto handler = vb::execution(vb::fs::path("/usr/bin/sed"), std::array{ std::string("s/[24680]/./") });
    auto data = std::array {
        std::pair{ "12345678", "1.3.5.7." },
        std::pair{ "abcef", "abcef" },
        std::pair{ "", ""},
        std::pair{ "a1b2", "a1b." }
    };

    auto load = handler.stdout_lines();
    auto reader = load.begin();
    for (auto [ send, recieve ] : data)
    {
        handler.send_line(send);
        auto read = *reader;
        ++reader;
        CHECK(read == recieve);
    }
}
#endif
