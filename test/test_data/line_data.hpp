// line_data.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_LINE_DATA_HPP
#define INCLUDED_LINE_DATA_HPP

#include <array>
#include <string_view>

namespace data {
using namespace std::literals;
constexpr auto add =
    std::array{ "1111\n2222\n3333\n"sv, "4444\n5555\n6666\n"sv, "7777\n8888\n"sv, "9999\n"sv, "0000\n"sv };

constexpr auto lines = std::array{ "1111\n"sv, "2222\n"sv, "3333\n"sv, "4444\n"sv, "5555\n"sv,
                                   "6666\n"sv, "7777\n"sv, "8888\n"sv, "9999\n"sv, "0000\n"sv };
}

#endif
