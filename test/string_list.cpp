// string_list.cpp                                                                        -*-C++-*-
#include "util/string_list.hpp"
#if 0
#include <catch2/catch_all.hpp>

TEST_CASE("", "[string_list]")
{
    static constexpr auto list=vb::splited<"a,ab,abc">;
    STATIC_REQUIRE(list.size() == 3);
    STATIC_REQUIRE(list[0] == "a");
    STATIC_REQUIRE(list[1] == "ab");
    STATIC_REQUIRE(list[2] == "abc");
}

#endif
