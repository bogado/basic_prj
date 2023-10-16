// primes.cpp                                                                        -*-C++-*-
#include "util/generator.hpp"
#include <iostream>
#include <array>

vb::generator<unsigned> primes(unsigned start)
{
    static constexpr auto small = std::array{2u,3u,5u,7u,11u,13u};
    if (start <= small.back())
    {
        for (auto i: small)
        {
            if (i >= start) {
                co_yield i;
            }
        }
    }
    start = small.back() + 2;
    while(true) {
        bool prime = true;
        unsigned top = static_cast<unsigned>(std::sqrt(start))+1;
        for (unsigned v = 3; v <= top && v <= start; v+=2) {
            if (start % v == 0 ) {
                prime = false;
                break;
            }
        }
        if (prime)
        {
            co_yield start;
        }
        start += 2;
    }
}

int main()
{
    for (auto i : primes(2)) {
        std::cout << "[" << i << "]\n";
    }
}

