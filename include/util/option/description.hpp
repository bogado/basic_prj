#ifndef INCLUDED_DESCRIPTION_HPP
#define INCLUDED_DESCRIPTION_HPP

#include "../string.hpp"
#include "util/option/concepts.hpp"

#include <string_view>

namespace vb::opt {
namespace description {

struct definition_parser
{
    using view                         = std::string_view;
    using size_type                    = std::size_t;
    static constexpr char no_short_key = '\0';

    view key_prefix           = "--";
    char short_key_prefix     = '-';
    char definition_separator = ':';

private:
    static constexpr bool is_word(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    constexpr view locate_end(view::const_iterator start, view definition) const noexcept
    {
        auto finish = std::end(definition);
        return view{ start, std::ranges::find_if(start, finish, [&](auto c) {
                         return !is_word(c) || c == short_key_prefix || c == definition_separator;
                     }) };
    }

public:
    constexpr size_type key_prefix_size() const { return key_prefix.size(); }

    constexpr size_type short_key_prefix_size() const { return 1z; }

    constexpr view locate_key(view definition) const
    {
        auto [start, finish] = std::ranges::search(definition, key_prefix);
        return locate_end(finish, definition);
    }

    constexpr view locate_description(view definition) const
    {
        auto start = std::ranges::find(definition, definition_separator);
        return locate_end(start, std::end(definition));
    }

    constexpr char locate_shortkey(view definition) const
    {
        auto start = std::ranges::adjacent_find(
            definition, [&](auto a, auto b) { return a == short_key_prefix && b != short_key_prefix; });

        if (start == std::end(definition)) {
            return no_short_key;
        }
        return *std::next(start);
    }
};

static constexpr auto unix_option_parser = definition_parser{};

struct basic
{
    using size_type     = std::size_t;
    using position_type = std::size_t;

private:
    std::string_view         my_definition;
    std::string_view         my_key;
    std::string_view         my_description;
    char                     my_shortkey;
    const definition_parser& my_parser = unix_option_parser;

public:
    explicit constexpr basic(is_string auto dta, const definition_parser& parser = unix_option_parser)
        : my_definition{ as_string_view(dta) }
        , my_key{ parser.locate_key(my_definition) }
        , my_description{ parser.locate_description(my_definition) }
        , my_shortkey{ parser.locate_shortkey(my_definition) }
        , my_parser{ parser }
    {
    }

    explicit constexpr basic(
        const char              *dta,
        size_type                size,
        const definition_parser& parser = unix_option_parser)
        : basic{ std::string_view{ dta, std::next(dta, static_cast<std::ptrdiff_t>(size)) }, parser }
    {
    }

    constexpr auto key() const -> std::string_view { return my_key; }

    constexpr auto shortkey() const -> char { return my_shortkey; }

    constexpr auto description() const -> std::string_view { return my_description; }

    constexpr bool operator==(const basic& other) const
    {
        return other.my_shortkey == my_shortkey && other.my_key == my_key && other.my_description == my_description;
    }

    constexpr bool operator!=(const basic&) const = default;

    constexpr bool operator==(const std::string_view& view) const
    {
        return (view.starts_with(my_parser.key_prefix) && view.substr(my_parser.key_prefix_size()) == key()) ||
            (view.size() >= (my_parser.short_key_prefix_size() + 1)
                && view.starts_with(my_parser.short_key_prefix)
                && view[my_parser.short_key_prefix_size()] == shortkey()
            );
    }

    constexpr bool operator==(const char& abv) const { return shortkey() == abv; }

    constexpr bool operator!=(const auto& b) const { return !(*this == b); }

    constexpr bool operator!=(const std::string_view& view) const { return !(*this == view); }

    constexpr auto key_prefix() const noexcept { return my_parser.key_prefix; }

    constexpr auto short_prefix() const noexcept { return my_parser.short_key_prefix; }
};

namespace test {
using namespace std::literals;
static constexpr auto test_opt = basic{ "--test, -t: Testing"sv };
static_assert(test_opt.shortkey() == 't');
static_assert(test_opt.key() == "test");
static_assert(test_opt == 't');
static_assert(test_opt.short_prefix() == '-');
static_assert(test_opt.key_prefix() == "--");
static_assert(test_opt == std::string_view("--test"));
static_assert(test_opt == std::string_view("-t"));
static_assert(test_opt != std::string_view("--another"));
static_assert(test_opt != std::string_view("-a"));
static_assert(is_description<decltype(test_opt)>);

} // namespace test
} // namespace literals
} // namespace vb::opt

#endif // INCLUDED_DESCRIPTION_HPP
