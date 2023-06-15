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

// NOLINTBEGIN modernize-avoid-c-arrays
//
template<size_t N>
struct static_string {

    consteval explicit static_string(const char (&str)[N]) // NOLINT(noExplicitConstructor)
    : value{}
    {
        std::copy_n(str, N, std::begin(value));
    }

    constexpr operator std::string_view() const {
        return std::string_view(value.data(), size());
    }

    constexpr auto size() const {
        return N - 1*(value.back() == 0);
    }

    std::array<char, N> value;
};

namespace literals {
    template <char ... DATA>
    consteval auto operator""_str() {
        return static_string<sizeof...(DATA)>(DATA...);
    }
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

#endif

