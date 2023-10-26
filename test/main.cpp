// main.cpp                                                                        -*-C++-*-
#include "util/execution.hpp"
#include "util/environment.hpp"

#include <iostream>
#include <iterator>
#include <string>
#include <algorithm>

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
        }
    }
    std::cout << "\n";
    return true;
}

using namespace std::literals;

int main()
{
    static constexpr auto home = vb::env{"HOME"};
    try {
        test_buffer();
        auto handler = vb::execution(vb::fs::path("/usr/bin/find"sv), std::array{home.value_or("."s), "-name"s, "*.hpp"s});
        for (auto line : handler.stdout_lines()) {
            std::cout << "→ " << line << "\n";
        }
        std::cout << "Exit: " <<  handler.wait() << "\n";
    } catch (std::exception& e) {
        std::cerr << "Caught exception : " << e.what() <<"\n";
    }
}
