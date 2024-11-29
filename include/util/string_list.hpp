#ifndef STRING_LIST_HPP_INCLUDED
#define STRING_LIST_HPP_INCLUDED

#include "./string.hpp"

#include <algorithm>
#include <iterator>
#include <ranges>

namespace vb {

// NOLINTEND modernize-avoid-c-arrays //
template <std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template <static_string STR, auto SEPARATOR = ','>
constexpr auto splited = []() {
    constexpr auto view =  std::string_view {STR};
    constexpr auto separators = view | std::views::filter([](auto c) { return c == SEPARATOR; 
    constexpr auto count = std::ranges::distance(separators.begin(), separators.end());

    static auto result = std::array<std::string_view, std::size(STR)>{};

    auto strings = view | std::views::split(SEPARATOR) | std::views::transform([](const auto& range) {
        return std::string_view(std::ranges::data(range), std::ranges::size(range));
    });

    auto end = std::ranges::copy(strings, std::begin(result));
    return std::span{std::begin(STR), end};
}();

namespace test {
using namespace std::literals;
    static constexpr auto splited_test = splited<"test,a,b,c">;
    static constexpr auto first = splited_test[0];

    static_assert(std::same_as<decltype(first), const std::string_view>);
    static_assert(std::size(splited_test) == 4);
    static_assert(first == "test"sv);
    static_assert(splited_test[1] == "a"sv);
    static_assert(splited_test[2] == "b"sv);
    static_assert(splited_test[3] == "c"sv);
}  // namespace test

}
#endif
