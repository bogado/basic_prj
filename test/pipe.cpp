#include <catch2/catch_all.hpp>

#include <util/pipe.hpp>
#include <util/system.hpp>

#include <iostream>

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

TEST_CASE("pipe redirection", "[pipe][buffer]")
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

TEST_CASE("pipe writing and reading", "[pipe][buffer]")
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
