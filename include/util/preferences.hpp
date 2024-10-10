#ifndef PREFFERENCES_HPP_INCLUDED
#define PREFFERENCES_HPP_INCLUDED

#include <algorithm>
#include <complex>
#include <optional>
#include <ranges>
#include <cctype>
#include <string_view>
#include "string.hpp"

namespace vb {

template <std::size_t SIZE, auto LONG_PREFIX = static_string{"--"}, auto SHORT_PREFIX = '-'>
requires (std::size(LONG_PREFIX) == 2)
struct argument {
    static constexpr auto long_option_prefix = LONG_PREFIX.view();
    static constexpr auto short_option_prefix = SHORT_PREFIX;

    static_string<SIZE> description;
    std::string_view long_option;
    char short_option;

    constexpr argument(const char(&desc)[SIZE]) :
        description{desc},
        long_option{fecth_long_option()},
        short_option{fetch_short_option()}
    {}

private:
    static constexpr auto isalpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c >= 'Z'); }
    static constexpr auto isnum(char c) { return (c >= '0' && c <= '9'); }
    static constexpr auto isalnum(char c) { return isalpha(c) || isnum(c); };
    static constexpr auto isspace(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }

    constexpr auto fetch_short_option() {
        if (description.front() == short_option_prefix && isalnum(description[1])) {
            return description[1];
        }
        auto short_opt = description | std::views::drop_while([](auto chr) {
            return chr != short_option_prefix;
        }) | std::views::drop(1) | std::views::take(1);

        if (short_opt.empty()) {
            return '\0';
        }

        return short_opt.front();
    }

    constexpr auto fecth_long_option() {
        auto start  = std::ranges::search(description, long_option_prefix).end();

        if (start == std::end(description)) {
            return std::string_view{};
        }

        return std::string_view(start, std::find_if_not(start, std::end(description), [](auto chr) { return isalnum(chr);} ));
    }
};

namespace test {
    static_assert(argument{"-a, --argument : test"}.short_option == 'a');
    static_assert(argument{"-a, --argument : test"}.long_option == "argument");
}

}

#endif // PREFFERENCES_HPP_INCLUDED
