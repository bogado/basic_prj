// main.cpp                                                                        -*-C++-*-
#include "util/execution.hpp"

int main()
{
    try {
    for (auto line : vb::run_file(vb::fs::path("/bin/ls"), std::string("."))) {
        std::cout << "→ " << line;
    }
    } catch (std::exception& e) {
        std::cerr << "Caught : " << e.what() <<"\n";
    }
}
