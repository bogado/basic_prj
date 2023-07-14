#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

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
static constexpr auto static_view = std::basic_string_view<typename decltype(STR)::value_type, typename decltype(STR)::traits_type>{STR};


}  // namespace vb

#endif

