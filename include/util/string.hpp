#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>
#include <array>

namespace vb {

inline namespace constexpr_str {

// NOLINTBEGIN modernize-avoid-c-arrays
template<size_t N>
struct static_string {
    consteval static_string(const char (&str)[N])
    : value{}
    {
        std::copy_n(str, N, std::begin(value));
    }

    template <std::same_as<char> ... ARG_Ts>
    consteval static_string(ARG_Ts... args)
    : value(args...)
    {}

    constexpr operator std::string_view() const {
        return std::string_view(value.data(), size());
    }

    constexpr auto size() const {
        return N - 1*(value.back() == 0);
    }

    std::array<char, N> value;
};

template <std::same_as<char> ... ARGs>
static_string(ARGs...) -> static_string<sizeof...(ARGs)>;

template <char ... DATA>
consteval auto operator""_str() {
    return static_string<sizeof...(DATA)>(DATA...);
}
// NOLINTEND modernize-avoid-c-arrays //

template <std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template <static_string STR>
static constexpr auto str_view = std::string_view(STR);

template <static_string STR, auto SEPARATOR = ','>
static constexpr auto split_count = std::ranges::count(str_view<STR>, SEPARATOR) + 1;

template <static_string STR, auto SEPARATOR = ','>
using splited_type = static_string_list<split_count<STR, SEPARATOR>>;  

template <static_string STR, auto SEPARATOR = ','>
constexpr auto splited = []() {
    constexpr auto view = str_view<STR>;
    auto strings = view | std::views::split(SEPARATOR) | std::views::transform([](const auto& range) {
        return std::string_view(std::ranges::data(range), std::ranges::size(range));
    });

    auto result = splited_type<STR, SEPARATOR>{};

    std::ranges::copy(strings, std::begin(result));
    return result;
}();

}

namespace test
{
    using namespace std::literals;

    static constexpr auto splited_test = splited<"test,a,b,c">;
    static constexpr auto first = splited_test[0];

    static_assert(std::same_as<decltype(first), const std::string_view>);
    static_assert(std::size(splited_test) == 4);
    static_assert(first == "test"sv);
    static_assert(splited_test[1] == "a"sv);
    static_assert(splited_test[2] == "b"sv);
    static_assert(splited_test[3] == "c"sv);
}

}

#endif

