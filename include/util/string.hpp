#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

#include <array>
#include <limits>
#include <string>
#include <string_view>

namespace vb {

template <typename STRING_TYPE>
concept is_string_type = std::same_as<std::char_traits<typename STRING_TYPE::value_type>, typename STRING_TYPE::traits_type> ||
    std::is_array_v<STRING_TYPE>;

// NOLINTBEGIN modernize-avoid-c-arrays
template <typename VALUE_T, typename TRAITS = std::char_traits<VALUE_T>>
struct basic_static_string {
	using traits_type      = TRAITS;
	using value_type       = VALUE_T;
	using string_type      = std::basic_string<value_type, traits_type>;
	using string_view_type = std::basic_string_view<value_type, traits_type>;
	using value_limits     = std::numeric_limits<value_type>;

    const VALUE_T* content;
    std::size_t length;

    template <std::size_t LENGTH>
	consteval basic_static_string(const value_type (&s)[LENGTH])
	    : content(s)
        , length(LENGTH)
    {}

    consteval basic_static_string(const value_type* s, std::size_t len)
        : content(s)
        , length(len)
    {}

	constexpr operator string_view_type() const {
		return std::string_view{content, length};
	}

	constexpr auto size() const { return length; }

    constexpr auto operator<=>(const basic_static_string& other) const = default;
    constexpr bool operator==(const basic_static_string& other) const = default;
    constexpr bool operator!=(const basic_static_string& other) const = default;
};

using static_string = basic_static_string<char>;

namespace literals {

auto consteval operator ""_str(const char* data, std::size_t len)
{
    return static_string{data, len};
}

}

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

