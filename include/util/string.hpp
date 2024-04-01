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
template <std::size_t LENGTH>
requires (LENGTH > 0)
struct static_string {
	using traits_type      = std::char_traits<char>;
	using value_type       = traits_type::char_type;
    using size_type        = traits_type::pos_type;
	using string_type      = std::basic_string<value_type, traits_type>;
	using string_view_type = std::basic_string_view<value_type, traits_type>;
	using value_limits     = std::numeric_limits<value_type>;

    static constexpr auto length = LENGTH;

    std::array<value_type, length> content;

    // No escape from c arrays here
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,cppcoreguidelines-avoid-c-arrays) 
	constexpr static_string(const value_type (&s)[LENGTH])
    {
        std::copy_n(std::begin(s), LENGTH, std::begin(content));
    }

	constexpr auto view() const 
        -> string_view_type
    {
		return string_view_type{content.data(), LENGTH};
	}

	constexpr auto size() const { return LENGTH - (*std::rbegin(content) == '\0'? 1: 0); }

    constexpr auto operator<=>(const static_string& other) const
    {
        return view() <=> other.view();
    }

    constexpr bool operator==(const static_string& other) const = default;
    constexpr bool operator!=(const static_string& other) const = default;
};

namespace test {

static constexpr auto test = static_string("123");
static_assert(test.size() == 3); // The final '\0' is included.

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

