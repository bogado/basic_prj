// options.cpp                                                                        -*-C++-*-

#include "util/options.hpp"

#include <catch2/catch_all.hpp>
namespace vb::opt {
using namespace std::literals;

TEST_CASE("Test options parsing", "[options]")
{
    auto option = "-t, --test Testing the optrions"_opt;
    REQUIRE(option.shortkey() == 't');
    REQUIRE(option.key() == "test"sv);
}

} // namespace vb::opt
