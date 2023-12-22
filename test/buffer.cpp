// buffer.cpp                                                                        -*-C++-*-

#include <array>
#include <catch2/catch_all.hpp>

#include "util/buffer.hpp"
#include "test_data/line_data.hpp"

#include <algorithm>
#include <iterator>

TEST_CASE("Buffer", "[buffer][generator]")
{
    auto reader = [](auto&& message) {
        return [message](auto load, auto sz) {
            auto end = std::copy_n(std::begin(message), std::min(std::size(message), sz), load);
            return std::distance(load, end);
        };
    };

    static constexpr std::size_t BIG = vb::sys::PAGE_SIZE;

    auto hello = vb::buffer_type();

    hello.load(reader("Hello\n"));
    REQUIRE(hello.unload_line() == std::string("Hello\n"));

    hello.load(reader("world\n!!\n"));
    REQUIRE(hello.unload_line() == std::string("world\n"));
    REQUIRE(hello.unload_line() == "!!\n");

    REQUIRE(hello.load(reader(std::string(BIG, '-'))) == vb::sys::PAGE_SIZE);
    REQUIRE(hello.unload_line() == std::string(vb::sys::PAGE_SIZE, '-'));
}
