#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>
#include <array>

namespace vb {

template <typename STRING_TYPE>
concept is_string_type = std::same_as<std::char_traits<typename STRING_TYPE::value_type>, typename STRING_TYPE::traits_type> ||
    std::is_array_v<STRING_TYPE>;

inline namespace constexpr_str {

consteval auto operator""_str(const char* str, std::size_t len)
{
    return std::string_view(str, len);
}

// NOLINTEND modernize-avoid-c-arrays //
template <std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template <is_string_type auto STR, auto SEPARATOR = ','>
constexpr auto splited = []() {
    constexpr auto view =  std::string_view {STR};

    static auto result = std::array<std::string_view, std::size(STR)>{};

    auto strings = view | std::views::split(SEPARATOR) | std::views::transform([](const auto& range) {
        return std::string_view(std::ranges::data(range), std::ranges::size(range));
    });

    auto end = std::ranges::copy(strings, std::begin(result));
    return std::span{std::begin(STR), end};
}();

}

namespace test
{
    using namespace std::literals;

    static constexpr auto splited_test = splited<"test,a,b,c"_str>;
    static constexpr auto first = splited_test[0];

    static_assert(std::same_as<decltype(first), std::string_view>);
    static_assert(std::size(splited_test) == 4);
    static_assert(first == "test"sv);
    static_assert(splited_test[1] == "a"sv);
    static_assert(splited_test[2] == "b"sv);
    static_assert(splited_test[3] == "c"sv);
}

}

#endif

