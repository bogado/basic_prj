// string_list.cpp                                                                        -*-C++-*-
#include "util/string_list.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("", "[string_list]")
{
    static constexpr auto list=vb::splited<"a,ab,abc">;
    STATIC_REQUIRE(list.size() == 3);
    STATIC_REQUIRE(list[0] == "a");
    STATIC_REQUIRE(list[1] == "ab");
    STATIC_REQUIRE(list[2] == "abc");
}


// --------------------------------------------------------------
// NOTICE:
// Copyright 2024 Bloomberg Finance L.P. All rights reserved.
// Property of Bloomberg Finance L.P. (BFLP)
// This software is made available solely pursuant to the
// terms of a BFLP license agreement which governs its use
// ----------------------- END-OF-FILE --------------------------
