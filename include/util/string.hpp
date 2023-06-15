#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

#include <bits/utility.h>
#include <algorithm>
#include <array>
#include <cinttypes>
#include <limits>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace vb {

template <typename STRING_TYPE>
concept is_string_type = std::same_as<std::char_traits<typename STRING_TYPE::value_type>, typename STRING_TYPE::traits_type> ||
    std::is_array_v<STRING_TYPE>;

// NOLINTBEGIN modernize-avoid-c-arrays
template <typename VALUE_T, std::size_t LENGTH, typename TRAITS = std::char_traits<VALUE_T>>
struct basic_static_string {
	using traits_type      = TRAITS;
	using value_type       = VALUE_T;
	using string_type      = std::basic_string<value_type, traits_type>;
	using string_view_type = std::basic_string_view<value_type, traits_type>;
	using value_limits     = std::numeric_limits<value_type>;

    static constexpr auto length = LENGTH;

	consteval basic_static_string(const value_type (&s)[LENGTH])
	    : content()
    {
        std::copy(std::begin(s), std::end(s), std::begin(content));
    }

	constexpr operator string_view_type() {
		return string_view_type{content, size()};
	}

	constexpr auto size() const { return length; }

    constexpr auto operator<=>(const basic_static_string& other) const = default;
    constexpr bool operator==(const basic_static_string& other) const = default;
    constexpr bool operator!=(const basic_static_string& other) const = default;

    value_type content[LENGTH];
};

template <typename TYPE, std::size_t N>
basic_static_string(const TYPE(&)[N]) -> basic_static_string<TYPE, N>;

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

template<std::size_t N>
using static_string = basic_static_string<char, N>;
namespace test {

static constexpr auto test = basic_static_string("test");
static_assert(test.size() == 5); // The final '\0' is included.

}  // namespace literals

template <typename STRING>
concept is_string = requires {
    typename STRING::traits_type;
    typename STRING::value_type;
    typename STRING::traits_type::char_type;
    { std::same_as<typename STRING::traits_type::char_type, typename STRING::value_type> };
};

template <std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template <static_string STR>
static constexpr auto static_view = std::basic_string_view<typename STR::value_type, typename STR::traits_type>{STR};

template <static_string STR, auto SEPARATOR = ','>
static constexpr auto split_count =
    std::ranges::count(static_view<STR>, SEPARATOR) + 1;

template <static_string STR, auto SEPARATOR = ','>
using splited_type = static_string_list<split_count<STR, SEPARATOR>>;

template <static_string STR, auto SEPARATOR = ','>
constexpr auto splited = []() {
	constexpr auto view    = static_view<STR>;
	auto           strings = view | std::views::split(SEPARATOR) |
	               std::views::transform([](const auto& range) {
		               return std::string_view(std::ranges::data(range),
		                                       std::ranges::size(range));
	               });

	auto result = splited_type<STR, SEPARATOR>{};

	std::ranges::copy(strings, std::begin(result));
	return result;
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

}  // namespace vb

#endif

