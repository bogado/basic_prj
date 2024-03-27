#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

#include <array>
#include <concepts>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace vb {

template <typename STRING_TYPE>
concept is_string_type = std::same_as<std::char_traits<typename STRING_TYPE::value_type>, typename STRING_TYPE::traits_type> ||
    std::is_array_v<STRING_TYPE> || std::same_as<STRING_TYPE, const char *>;

// NOLINTBEGIN modernize-avoid-c-arrays
template <typename VALUE_T, std::size_t LENGTH, typename TRAITS = std::char_traits<VALUE_T>>
struct basic_static_string {
	using traits_type      = TRAITS;
	using value_type       = VALUE_T;
    using size_type        = std::size_t;
	using string_type      = std::basic_string<value_type, traits_type>;
	using string_view_type = std::basic_string_view<value_type, traits_type>;
	using value_limits     = std::numeric_limits<value_type>;

    static constexpr auto length = LENGTH;

    VALUE_T content[length];

	constexpr basic_static_string(const value_type (&s)[LENGTH])
    {
        std::copy_n(std::begin(s), LENGTH, std::begin(content));
    }

	constexpr explicit operator string_view_type() const {
		return std::string_view{content, LENGTH};
	}

	constexpr auto size() const { return LENGTH; }

    constexpr auto operator<=>(const basic_static_string& other) const
    {
        return static_cast<string_view_type>(*this) <=> static_cast<string_view_type>(other);
    }

    constexpr bool operator==(const basic_static_string& other) const = default;
    constexpr bool operator!=(const basic_static_string& other) const = default;
};

template <std::size_t SIZE>
struct static_string : basic_static_string<char, SIZE> 
{
    static_string(char (&str)[SIZE]) 
        : basic_static_string<char, SIZE>(str)
    {}
};


namespace test {

static constexpr auto test = basic_static_string("123");
static_assert(test.size() == 4); // The final '\0' is included.

}  // namespace literals

template <typename STRING>
concept is_string = requires {
    typename STRING::traits_type;
    typename STRING::value_type;
    typename STRING::traits_type::char_type;
    { std::same_as<typename STRING::traits_type::char_type, typename STRING::value_type> };
} || requires {
    std::is_array_v<STRING>;
    typename std::char_traits<std::remove_all_extents_t<STRING>>;
} || requires {
    std::is_pointer_v<STRING>;
    typename std::char_traits<std::remove_pointer_t<STRING>>;
};

template <std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template <static_string STR>
static constexpr auto static_view = std::basic_string_view<typename decltype(STR)::value_type, typename decltype(STR)::traits_type>{STR};

}  // namespace vb

#endif

