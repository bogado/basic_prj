// main.cpp                                                                        -*-C++-*-
#include "util/execution.hpp"

#include <iostream>
#include <string>
#include <algorithm>

bool test_buffer()
{
    auto reader = [](auto&& message) {
        return [message](auto load, auto) {
            std::copy(std::begin(message), std::end(message), load);
            return std::size(message);
        };
    };

    auto hello = vb::buffer_type();
    hello.load(reader("Hello\n"));
    std::cout << hello.unload_line() << " ";
    hello.load(reader("world\n!!\n"));
    while(hello.has_data()) {
        std::cout << hello.unload_line() << " ";
    }
    std::cout << "\n";
    return true;
}

using namespace std::literals;

int main()
{
    try {
        test_buffer();
        auto handler = vb::execution(vb::fs::path("/bin/find"sv), "."s, "-name"s, "*.hpp"s);
        for (auto line : handler.stdout_lines()) {
            std::cout << "→ " << line << "\n";
        }
        std::cout << "Exit: " <<  handler.exit_stat().get() << "\n";
    } catch (std::exception& e) {
        std::cerr << "Caught exception : " << e.what() <<"\n";
    }
}
