// primes.cpp                                                                        -*-C++-*-

#include <catch2/catch_all.hpp>

#include "catch2/catch_test_macros.hpp"
#include "util/generator.hpp"

#include <array>
#include <cmath>

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

TEST_CASE("Prime generator", "[Prime][generator]")
{
    constexpr std::array CHECK {
        2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u,
        41u, 43u, 47u, 53u, 59u, 61u, 67u, 71u, 73u, 79u, 83u,
        89u, 97u, 101u, 103u, 107u, 109u, 113u, 127u, 131u,
        137u, 139u, 149u, 151u, 157u, 163u, 167u, 173u, 179u,
        181u, 191u, 193u, 197u, 199u, 211u, 223u, 227u, 229u,
        233u, 239u, 241u, 251u, 257u, 263u, 269u, 271u, 277u,
        281u, 283u, 293u, 307u, 311u, 313u, 317u, 331u, 337u,
        347u, 349u, 353u, 359u, 367u, 373u, 379u, 383u, 389u,
        397u, 401u, 409u, 419u, 421u, 431u, 433u, 439u, 443u,
        449u, 457u, 461u, 463u, 467u, 479u, 487u, 491u, 499u,
        503u, 509u, 521u, 523u, 541u
    };
    auto check = CHECK.begin();
    auto numbers = primes(2);
    for (auto i : numbers) {
        REQUIRE(i == *check);
        if (++check == CHECK.end()) {
            break;
        }
    }

    BENCHMARK_ADVANCED("Prime generator")(Catch::Benchmark::Chronometer meter) {
        auto generator = primes(2);
        auto prime_gen = generator.begin();
        meter.measure([&](){
            ++prime_gen;
            return *prime_gen;
        });
    };
}

