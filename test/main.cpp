// main.cpp                                                                        -*-C++-*-
#include "util/execution.hpp"

#include <iostream>
#include <iterator>
#include <string>
#include <algorithm>
#include <source_location>

static constexpr auto test_dir = []() {
    auto file = std::string_view{std::source_location::current().file_name()};
    auto dir = file.substr(0, file.find_last_of('/'));
    return dir;
}();

bool test_buffer()
{
    auto reader = [](auto&& message) {
        return [message](auto load, auto sz) {
            auto end = std::copy_n(std::begin(message), std::min(std::size(message), sz), load);
            return std::distance(load, end);
        };
    };

    static constexpr std::size_t BIG = 1024u*5u;

    auto hello = vb::buffer_type();
    hello.load(reader("Hello\n"));
    std::cout << hello.unload_line() << " ";
    hello.load(reader("world\n!!\n"));
    bool has_added = false;
    hello.load(reader(std::string(BIG, '-')));
    while(hello.has_data()) {
        std::cout << hello.unload_line() << " ";
        if (!has_added) {
            hello.load(reader("end of the very large line\n"));
            has_added = true;
        }
    }
    std::cout << "\n";
    return true;
}

