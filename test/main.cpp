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
        for (auto line: vb::run_file(vb::fs::path("/bin/ls"sv), "."s, ".."s)) {
            std::cout << "→ " << line << "\n";
        }
    } catch (std::exception& e) {
        std::cerr << "Caught : " << e.what() <<"\n";
    }
}
