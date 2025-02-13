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

}
// --------------------------------------------------------------
// NOTICE:
// Copyright 2024 Bloomberg Finance L.P. All rights reserved.
// Property of Bloomberg Finance L.P. (BFLP)
// This software is made available solely pursuant to the
// terms of a BFLP license agreement which governs its use
// ----------------------- END-OF-FILE --------------------------
