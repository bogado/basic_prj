#ifndef INCLUDED_DESCRIPTION_HPP
#define INCLUDED_DESCRIPTION_HPP

#include "../optional.hpp"
#include "../string.hpp"

#include <string_view>
#include <concepts>
#include <algorithm>

namespace vb::opt {

template<typename OPTION_DESCRIPTION>
concept is_description = requires(const OPTION_DESCRIPTION value) {
    { value.key() } -> is_string;
    { value.shortkey() } -> std::same_as<char>;
    { value.description() } -> is_string;
    { value == 'a' } -> std::same_as<bool>;
    { value == std::string_view{} } -> std::same_as<bool>;
};

template<typename TYPE>
concept is_optional_description = is_optional<TYPE> && is_description<typename TYPE::value_type>;

template<typename PREFIX_T>
concept is_prefix_type = is_string<PREFIX_T> || std::same_as<PREFIX_T, char>;

enum class option_part : unsigned
{
    KEY,
    SHORT_KEY,
    VALUE,
    DESCRIPTION
};

template<typename PART>
concept is_string_part = is_string<PART> || std::same_as<PART, char>;

template<is_string_part PART>
constexpr auto find_part(std::string_view from, PART part)
{
    if constexpr (std::same_as<PART, char>) {
        return std::ranges::find(from, part);
    } else {
        auto part_view = as_string_view(part);
        return std::begin(std::ranges::search(from, part_view));
    }
}

template<is_string_part PART_T>
constexpr auto size_of_part(PART_T part)
{
    if constexpr (std::same_as<PART_T, char>) {
        return std::size_t{ 1 };
    } else {
        return std::size(part);
    }
}

template<
    is_string_part auto KEY_PREFIX            = static_string{ "--" },
    is_string_part auto SHORT_PREFIX          = '-',
    is_string_part auto VALUE_SEPARATOR       = '=',
    is_string_part auto DESCRIPTION_SEPARATOR = ':'>
struct option_parts_parser
{
private:
    template<typename T>
    static consteval decltype(auto) part_for(T&& value)
    {
        if constexpr (std::same_as<T, char>) {
            return std::forward<T>(value);
        } else {
            return as_string_view(std::forward<T>(value));
        }
    }

    constexpr static auto is_word_char(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
    }

public:
    static constexpr auto parts = std::tuple{ part_for(KEY_PREFIX),
                                              part_for(SHORT_PREFIX),
                                              part_for(VALUE_SEPARATOR),
                                              part_for(DESCRIPTION_SEPARATOR) };

    template<option_part PART>
    static constexpr auto part_prefix = std::get<std::to_underlying(PART)>(parts);

    template<option_part PART>
    using prefix_type = std::remove_const_t<std::tuple_element_t<std::to_underlying(PART), decltype(parts)>>;

    template<option_part PART>
    constexpr static auto prefix_size = []() {
        constexpr auto prefix = part_prefix<PART>;
        using prefix_type     = prefix_type<PART>;
        if constexpr (std::same_as<prefix_type, char>) {
            return 1uz;
        } else if constexpr (std::same_as<prefix_type, std::string_view>) {
            return std::size(prefix);
        } else {
            return sizeof(prefix_type);
        }
    }();

    template<option_part PART>
    constexpr static auto find_prefix(std::string_view from) -> std::string_view::const_iterator
    {
        return find_part(from, part_prefix<PART>);
    }

    template<option_part PART>
    constexpr static auto part(std::string_view from)
    {
        auto start = find_prefix<PART>(from);
        if (start == std::end(from) || std::next(start) == std::end(from)) {
            return std::string_view{};
        }
        start    = std::next(start, prefix_size<PART>);
        auto end = std::ranges::find_if_not(start, std::end(from), is_word_char);
        return std::string_view(start, end);
    }

    constexpr static auto short_key(std::string_view from) -> char
    {
        auto found = part<option_part::SHORT_KEY>(from);
        if (found.size() == 0) {
            return '\0';
        }
        return found[0];
    }
};

struct basic_description
{
    using enum option_part;
    using size_type = std::size_t;

    using position_type = std::size_t;

    std::string_view my_data;
    std::string_view my_key;
    std::string_view my_description;
    std::string_view my_key_prefix;
    std::string_view my_short_prefix;
    char             my_shortkey;

    template<typename PARTS_PARSER = option_parts_parser<>>
    explicit constexpr basic_description(is_string auto dta, PARTS_PARSER = {})
        : my_data{ as_string_view(dta) }
        , my_key{ PARTS_PARSER::template part<KEY>(my_data) }
        , my_description{ PARTS_PARSER::template part<DESCRIPTION>(my_data) }
        , my_key_prefix{ find_part(my_data, PARTS_PARSER::template part_prefix<option_part::KEY>),
                         size_of_part(PARTS_PARSER::template part_prefix<option_part::KEY>) }
        , my_short_prefix{ find_part(my_data, PARTS_PARSER::template part_prefix<option_part::SHORT_KEY>),
                           size_of_part(PARTS_PARSER::template part_prefix<option_part::SHORT_KEY>) }
        , my_shortkey{ PARTS_PARSER::short_key(my_data) }
    {
    }

    template<typename PARTS_PARSER = option_parts_parser<>>
    explicit constexpr basic_description(const char *dta, size_type size, PARTS_PARSER = {})
        : basic_description{ std::string_view{ dta, std::next(dta, static_cast<std::ptrdiff_t>(size)) } }
    {
    }

    constexpr auto key() const -> std::string_view { return my_key; }

    constexpr auto shortkey() const -> char { return my_shortkey; }

    constexpr auto description() const -> std::string_view { return my_description; }

    bool operator==(const basic_description&) const = default;
    bool operator!=(const basic_description&) const = default;

    constexpr bool operator==(const std::string_view& view) const
    {
        return (view.starts_with(my_key_prefix) && view.substr(my_key_prefix.size()) == key()) ||
               (view.starts_with(my_short_prefix) && view[my_short_prefix.size()] == shortkey());
    }

    constexpr bool operator==(const char& abv) const { return shortkey() == abv; }

    constexpr bool operator!=(const auto& b) const { return !(*this == b); }

    constexpr bool operator!=(const std::string_view& view) const { return !(*this == view); }

};

namespace literals {
constexpr auto operator""_opt(const char *opt, std::size_t size)
{
    return basic_description{ opt, size };
}

namespace test {
using namespace literals;
constexpr auto test_opt = "-t, --test : this is a test"_opt;
static_assert(test_opt.shortkey() == 't');
static_assert(test_opt.key() == "test");
static_assert(test_opt == 't');
static_assert(test_opt.my_short_prefix == "-");
static_assert(test_opt.my_key_prefix == "--");
static_assert(test_opt == std::string_view("--test"));
static_assert(test_opt == std::string_view("-t"));
static_assert(test_opt != std::string_view("--another"));
static_assert(test_opt != std::string_view("-a"));
static_assert(is_description<decltype(test_opt)>);

} // namespace test
} // namespace literals
} // namespace vb::opt

namespace vb::literals { using namespace vb::opt::literals; }

#endif // INCLUDED_DESCRIPTION_HPP
