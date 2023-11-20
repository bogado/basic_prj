// pipe.cpp                                                                        -*-C++-*-

#include "util/pipe.hpp"

#include <algorithm>
#include <iostream>

int main()
{
    auto reader = [](auto&& message) {
        return [message](auto load, auto sz) {
            auto end = std::copy_n(std::begin(message), std::min(std::size(message), sz), load);
            return std::distance(load, end);
        };
    };

    static constexpr std::size_t BIG = 1024z*5z;
    try {
        auto hello = vb::buffer_type();
        hello.load(reader("Hello\n"));
        std::cout << hello.unload_line() << " ";
        hello.load(reader("world\n!!\n"));
        bool has_added = false;
        hello.load(reader(std::string(BIG, '-')));
        while(hello.has_data()) {
            std::cout << hello.unload_line() << " ";
            if (!has_added) {
                std::cout << "\nloaded : " << hello.load(reader("end of the very large line\n")) << "\n";
                has_added = true;
            }
        }
        std::cout << "\n";
    } catch (std::exception& e) {
        std::cerr << "Caught exception : " << e.what() <<"\n";
    }
} 
