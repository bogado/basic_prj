#ifndef STRING_HPP_INCLUDED
#define STRING_HPP_INCLUDED

#include "./concept_helper.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

namespace vb {

template<typename STRING_TYPE>
concept is_string_type =
    static_same_as<std::char_traits<typename STRING_TYPE::value_type>, typename STRING_TYPE::traits_type> ||
    std::is_array_v<STRING_TYPE> || static_same_as<STRING_TYPE, const char *>;

// NOLINTBEGIN modernize-avoid-c-arrays
template<std::size_t LENGTH, typename VALUE_T = char, typename TRAITS = std::char_traits<VALUE_T>>
struct static_string
{
    using storage_type     = std::array<VALUE_T, LENGTH>;
    using traits_type      = TRAITS;
    using value_type       = VALUE_T;
    using char_type        = VALUE_T;
    using size_type        = std::size_t;
    using difference_type  = std::ptrdiff_t;
    using string_type      = std::basic_string<value_type, traits_type>;
    using string_view_type = std::basic_string_view<value_type, traits_type>;
    using value_limits     = std::numeric_limits<value_type>;
    using iterator         = storage_type::const_iterator;

    static constexpr auto length = LENGTH;

    storage_type content;

    // No escape from c arrays here
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,cppcoreguidelines-avoid-c-arrays)
    constexpr static_string(const value_type (&s)[LENGTH]) noexcept
    {
        std::copy_n(std::begin(s), LENGTH, std::begin(content));
    }

    constexpr auto view() const noexcept -> string_view_type { return string_view_type{ content.data(), size() }; }

    constexpr auto front() const noexcept
    {
        if constexpr (length == 0) {
            return '\0';
        }
        return content.front();
    }

    constexpr auto back() const noexcept
    {
        if constexpr (length == 0) {
            return '\0';
        }
        return content.back();
    }

    constexpr auto substr(size_type start, difference_type len = length) const noexcept
    {
        if (start >= size()) {
            return std::string_view{};
        }

        auto start_loc = std::next(std::begin(content), static_cast<difference_type>(start));
        return std::string_view{
            start_loc,
            std::next(start_loc, std::min<difference_type>(ssize() - static_cast<difference_type>(start), len))
        };
    }

    constexpr auto operator[](size_t pos) const noexcept
    {
        if (pos >= size()) {
            return '\0';
        }
        return content[pos];
    }

    /// The last non '\0' element.
    constexpr auto last() const noexcept
    {
        auto tail = content | std::views::reverse | std::views::drop_while([](auto chr) { return chr == '\0'; }) |
                    std::views::take(1);
        return tail.front();
    }

    constexpr auto size() const noexcept { return LENGTH - (*std::rbegin(content) == '\0' ? 1 : 0); }
    constexpr auto ssize() const noexcept { return static_cast<difference_type>(size()); }

    constexpr auto begin() const noexcept { return std::begin(content); }
    constexpr auto end() const noexcept { return std::end(content); }

    template<size_type OTHER_SIZE>
    constexpr auto operator<=>(const static_string<OTHER_SIZE>& other) const noexcept
    {
        return view() <=> other.view();
    }

    constexpr operator string_view_type() const { return view(); }

    constexpr auto array() const noexcept { return content; }

    template<size_type OTHER_SIZE>
        requires(OTHER_SIZE != LENGTH)
    constexpr bool operator==(const static_string<OTHER_SIZE>&) const noexcept
    {
        return false;
    }

    template<size_type OTHER_SIZE>
        requires(OTHER_SIZE != LENGTH)
    constexpr bool operator!=(const static_string<OTHER_SIZE>&) const noexcept
    {
        return true;
    }

    constexpr bool operator==(const static_string& other) const noexcept = default;
    constexpr bool operator!=(const static_string& other) const noexcept = default;
};

namespace test {

static constexpr auto test  = static_string("123");
static constexpr auto test2 = static_string("234");
static constexpr auto test3 = static_string("2345");
static_assert(test.size() == 3);
static_assert(test.substr(1, 1) == "2");
static_assert(test.back() == '\0');
static_assert(test.last() == '3');
static_assert(test >= test);
static_assert(test == "123");
static_assert(test != test2);
static_assert(test < test3);
static_assert(test3 > test);
static_assert(test < test2);
static_assert(test2 > test);
static_assert(test.view() != test2.substr(1));
static_assert(test3.substr(0, 3) == "234");
static_assert(test2.view() == test3.substr(0, 3));

} // namespace literals

template<typename CHAR>
concept is_char = std::same_as<char, std::remove_cv_t<CHAR>> || std::same_as<char8_t, std::remove_cv_t<CHAR>> ||
                  std::same_as<char16_t, std::remove_cv_t<CHAR>> || std::same_as<char32_t, std::remove_cv_t<CHAR>>;

template<typename STRING>
concept is_string_class = std::same_as<typename STRING::traits_type::char_type, typename STRING::value_type>;

template<typename STRING>
concept is_c_array_string = std::is_array_v<STRING> && is_char<std::remove_all_extents_t<STRING>>;

static_assert(is_c_array_string<char[3]>); // NOLINT(cppcoreguidelines-avoid-c-arrays)

template<typename STRING>
concept is_pointer_string = std::is_pointer_v<STRING> && is_char<std::remove_pointer_t<STRING>>;

template<typename STRING>
concept is_string = is_string_class<std::remove_cvref_t<STRING>> || is_c_array_string<std::remove_cvref_t<STRING>> || is_pointer_string<std::remove_cvref_t<STRING>>;

static_assert(!is_string<int>);

constexpr auto
as_string_view(const is_string auto& str)
{
    return std::string_view(str);
}

template<std::size_t N>
using static_string_list = std::array<std::string_view, N>;

} // namespace vb

#endif
