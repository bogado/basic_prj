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


using namespace std::literals;

int main()
{
    try {
        auto handler = vb::execution(vb::fs::path("/usr/bin/find"sv), std::array{std::string{test_dir}, "-print"s});
        for (auto line : handler.stdout_lines()) {
            std::cout << "→ " << line << "\n";
        }
        std::cout << "Exit: " <<  handler.wait() << "\n";
    } catch (std::exception& e) {
        std::cerr << "Caught exception : " << e.what() <<"\n";
    }
}
