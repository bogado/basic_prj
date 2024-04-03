#include "test_data/line_data.hpp"

#include <catch2/catch_all.hpp>

#include <util/pipe.hpp>
#include <util/system.hpp>

#include <iostream>

namespace Catch {

template<>
struct StringMaker<vb::pipe::expect_string>
{
    static inline std::string convert(vb::pipe::expect_string expected_str)
    {
        static constexpr auto quoted = [](std::string a) {
            auto quote = std::string("\"");
            return a.insert(0, quote) + quote;
        };
        return expected_str
            ? quoted(expected_str.value())
            : std::string("Ø → Fail");
    }
};

}

struct save_fd {
    int old_fd = -1;
    int fd = -1;

    save_fd(int old_fd_) :
        old_fd(old_fd_),
        fd(vb::sys::dup(old_fd))
    {}

    save_fd(const save_fd&) = delete;
    save_fd& operator=(const save_fd&) = delete;
    save_fd(const save_fd&&) = delete;
    save_fd& operator=(const save_fd&&) = delete;


    ~save_fd()
    {
        ::dup2(fd, old_fd);
    }
};

struct io_direction_tests {
    vb::io_direction main;
    vb::io_direction negative;
};

TEST_CASE("direction setup", "[pipe][direction]")
{
    using enum vb::io_direction;
    auto direction = GENERATE(
        io_direction_tests{ READ, WRITE },
        io_direction_tests{ WRITE, READ },
        io_direction_tests{ BOTH,  NONE },
        io_direction_tests{ NONE,  BOTH }
    );
    REQUIRE((!direction.main) == direction.negative); 
    SECTION("setting direction") {
        auto pipe = vb::pipe{};
        pipe.set_direction(direction.main);
        REQUIRE(pipe.direction() == direction.main);
    }

    SECTION("closing direction") {
        auto pipe = vb::pipe{};
        pipe.close(direction.main);
        REQUIRE(pipe.direction() == direction.negative);
    }
}

TEST_CASE("pipe redirection", "[pipe][generator][buffer]")
{
    save_fd restore(2);
    vb::pipe pipe_test{};

    pipe_test.redirect_err();

    std::cerr << "Test\n";
    REQUIRE(pipe_test() == std::string("Test\n"));
    REQUIRE_FALSE(pipe_test.has_data());

    std::cerr << "1 2 3\n4 5 6\n";
    REQUIRE(pipe_test() == std::string("1 2 3\n"));
    REQUIRE(pipe_test.has_data());
    REQUIRE(pipe_test() == std::string("4 5 6\n"));
    REQUIRE_FALSE(pipe_test.has_data());
}

TEST_CASE("pipe writing and reading", "[pipe][generator][buffer]")
{
    vb::pipe pipe_test{};

    REQUIRE_FALSE(pipe_test.has_data());

    SECTION("Write single line and read it") {
        pipe_test("Test");
        REQUIRE(pipe_test.has_data());

        REQUIRE(pipe_test() == std::string("Test\n"));
        REQUIRE_FALSE(pipe_test.has_data());
    }

    SECTION("Write two lines and read them") {
        pipe_test("1 2 3\n4 5 6");
        REQUIRE(pipe_test() == std::string("1 2 3\n"));
        REQUIRE(pipe_test.has_data());
        REQUIRE(pipe_test() == std::string("4 5 6\n"));
        REQUIRE_FALSE(pipe_test.has_data());
    }
}

TEST_CASE("Read loop", "[pipe][buffer][generator]")
{
    vb::pipe pipe_test{};

    auto add_it = std::begin(data::add);
    auto expected_it = std::begin(data::lines);
    auto prev_ex = std::string("");
    while (add_it != std::end(data::add) && expected_it != std::end(data::lines))
    {
        INFO("Buffer : " << pipe_test);
        INFO("Next data: " << *add_it);
        INFO("Prev Expectation: " << prev_ex);
        INFO("Next Expectation: " << *expected_it);
        if (!pipe_test.has_data()) {
            REQUIRE(add_it != std::end(data::add));
            pipe_test(*add_it);
            add_it++;
        } else {
            prev_ex = *expected_it;
            REQUIRE(expected_it != std::end(data::lines));
            REQUIRE(pipe_test() == prev_ex);
            expected_it++;
        }
    }
}


