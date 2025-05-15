#ifndef INCLUDED_DESCRIPTION_HPP
#define INCLUDED_DESCRIPTION_HPP

#include "../string.hpp"

#include <string_view>

namespace vb::opt {
namespace description {

struct definition_parser {
    using view = std::string_view;
    static constexpr char no_short_key = '\0';
    
    view key_prefix = "--";
    char short_key_prefix = '-';
    char definition_separator = ':';

private:
    constexpr view locate_end(view::const_iterator start, view definition) const noexcept
    {
        auto finish = std::end(definition);
        return view{start, std::ranges::find_if(finish, std::end(definition), [&](auto c) {
            return std::isspace(c) || c == short_key_prefix || c == definition_separator;
        })};
    }

public:
    constexpr view locate_key(view definition) const {
        auto [start, finish] = std::ranges::search(definition, key_prefix);
        return locate_end(finish, definition);
    }

    constexpr view locate_description(view definition) const {
        auto start = std::ranges::find(definition, definition_separator);
        return locate_end(start, std::end(definition));
    }

    constexpr char locate_shortkey(view definition) const {
        auto start = std::ranges::adjacent_find(definition, [&](auto a, auto b) {
            return a == short_key_prefix && b != short_key_prefix;
        });

        if (start == std::end(definition)) {
            return no_short_key;
        }
        return *std::next(start);
    }
};

static constexpr auto unix_option_parser = definition_parser{};

struct basic_description
{
    using size_type = std::size_t;
    using position_type = std::size_t;

private:
    std::string_view my_definition;
    std::string_view my_key;
    std::string_view my_description;
    char             my_shortkey;
    const definition_parser& my_parser = unix_option_parser;

    explicit constexpr basic_description(is_string auto dta, const definition_parser& parser=unix_option_parser)
        : my_definition{ as_string_view(dta) }
        , my_key{ parser.locate_key(my_definition) }
        , my_description{ parser.locate_description(my_definition) }
        , my_shortkey{ parser.locate_shortkey(my_definition) }
        , my_parser{parser}
    {
    }

    explicit constexpr basic_description(const char *dta, size_type size, const definition_parser& parser = unix_option_parser)
        : basic_description{ std::string_view{ dta, std::next(dta, static_cast<std::ptrdiff_t>(size)) }, parser }
    {
    }

    constexpr auto key() const -> std::string_view { return my_key; }

    constexpr auto shortkey() const -> char { return my_shortkey; }

    constexpr auto description() const -> std::string_view { return my_description; }

    bool operator==(const basic_description&) const = default;
    bool operator!=(const basic_description&) const = default;

    constexpr bool operator==(const std::string_view& view) const
    {
        return (view.starts_with(my_parser.key_prefix) && view.substr(my_parser.key_prefix.size()) == key()) ||
               (view.starts_with(my_parser.short_key_prefix) && view[my_short_prefix.size()] == shortkey());
    }

    constexpr bool operator==(const char& abv) const { return shortkey() == abv; }

    constexpr bool operator!=(const auto& b) const { return !(*this == b); }

    constexpr bool operator!=(const std::string_view& view) const { return !(*this == view); }
};

namespace test {
using namespace std::literals;
static constexpr auto test_opt = basic_description{"--test, -t: Testing"sv};
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
