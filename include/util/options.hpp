// options.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_OPTIONS_HPP
#define INCLUDED_OPTIONS_HPP

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "util/string.hpp"
#include "util/converters.hpp"

namespace vb::opt {

template <typename OPTION_DESCRIPTION>
concept is_option_description = std::equality_comparable_with<OPTION_DESCRIPTION, std::string_view> &&
std::equality_comparable_with<OPTION_DESCRIPTION, char> && requires(const OPTION_DESCRIPTION value) {
    { value.key() } -> is_string;
    { value.shortkey() } -> std::same_as<char>;
    { value.description() } -> is_string;
};

template <typename OPTION_GROUP>
concept is_option_group = std::ranges::range<typename OPTION_GROUP::value_type>
&& is_option_description<typename OPTION_GROUP::value_type>
&& requires(const OPTION_GROUP group) {
    { group.name() } -> is_string;
    { group.description() } -> is_string;
};

template <typename PREFIX_T>
concept is_prefix_type = is_string<PREFIX_T> || std::same_as<PREFIX_T, char>;

template <std::size_t SIZE, char SHORT_PREFIX = '-', static_string KEY_PREFIX = "--", char DESCRIPTION_SEPARATOR = ':'>
struct opt_description {
    struct prefix {
        static constexpr auto short_key = SHORT_PREFIX;
        static constexpr auto key = KEY_PREFIX;
        static constexpr auto description = DESCRIPTION_SEPARATOR;

        constexpr static auto is_word_char(char c) {
                return 
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    (c == '_');
        }

        template <is_prefix_type PREFIX_T>
        constexpr static auto length(PREFIX_T prefix) {
            if constexpr (std::same_as<PREFIX_T, char>) {
                return 1;
            } else if constexpr (is_string<PREFIX_T>) {
                return std::size(prefix);
            } else {
                return sizeof(prefix);
            }
        }

        constexpr static auto find_pos(std::string_view from, auto prefix) {
            auto view = as_string_view(from);
            auto pos = std::size_t{0};
            while(pos < view.size() - 2 && is_word_char(view[pos+1]))
            {
                pos = view.find(prefix);
                view = view.substr(pos);
            }
            return pos; 
        }

        constexpr static auto find_after(std::string_view from, auto prefix) {
            auto view = as_string_view(from);
            view = view.substr(find_pos(from, prefix) + length(prefix));
            return view.substr(0, std::distance(std::begin(view), std::ranges::find_if_not(view, is_word_char)));
        }
    };

    using position_type = std::size_t;

    static_string<SIZE> data;

    constexpr opt_description(static_string<SIZE> dta)
        : data{dta}
    {}

    constexpr auto key() const 
    {
        return prefix::find_after(data, prefix::key);
    }

    constexpr auto shortkey() const 
    {
        auto key = prefix::find_after(data, prefix::short_key);
        return key.empty() ? '\0' : key[0];
    }

    constexpr auto description() const 
    {
        return ;
    }

    constexpr bool operator== (const is_string auto& k) const
    {
        auto view = to_string_view(k);
        return view.starts_with(prefix::key) && key() == view.substr(std::size(prefix::key));
    }

    constexpr bool operator== (char abv)
    {
        return shortkey() == abv;
    }
    
    template <typename T>
    requires(is_string<T> || std::same_as<T, char>)
    constexpr friend bool operator== (const T& a, const opt_description& b)
    {
        return b == a;
    }

    template <typename T>
    requires(is_string<T> || std::same_as<T, char>)
    constexpr friend bool operator!= (const T& a, const opt_description& b)
    {
        return !(b == a);
    }

    template <typename T>
    requires(is_string<T> || std::same_as<T, char>)
    constexpr bool operator!= (const T& a) {
        return !(*this == a);
    }
};

template <static_string OPT>
constexpr auto operator""_opt()
{
    return opt_description<decltype(OPT)::length>{OPT};
}

namespace test {
    constexpr auto test_opt = "-t, --test : this is a test"_opt;
    static_assert(test_opt.shortkey() == 't');
}

template <typename OPTION_T>
concept is_option = is_option_description<OPTION_T> 
&& requires (const OPTION_T value) {
    typename OPTION_T::value_type;
    { OPTION_T::parse(std::string_view("")) } -> std::same_as<std::optional<OPTION_T>>;
};

template <std::constructible_from<> VALUE_T>
auto default_builder() 
-> VALUE_T 
{ return {}; }

template<typename VALUE_T, is_option_description OPTION_DESCRIPTION_T>
struct basic_option : OPTION_DESCRIPTION_T {
    using value_type = VALUE_T;
    using option_description_type = OPTION_DESCRIPTION_T;

    template<typename... ARG_Ts>
        requires(std::constructible_from<option_description_type, ARG_Ts...>)
    constexpr basic_option(ARG_Ts&&...args) noexcept
      : option_description_type{ std::forward<ARG_Ts>(args)... }
    {
    }

    using option_description_type::key;
    using option_description_type::description;
    using option_description_type::shortkey;

    std::reference_wrapper<value_type> storage;
    bool updated = false;

    constexpr auto parse(std::string_view str)
    {
        if constexpr (parseable<value_type>) {
            return from_string<value_type>(str);
        }
    }
};

template <typename VALUE_T, is_option_description DESCRIPTION_TYPE>
constexpr auto opt(DESCRIPTION_TYPE&& description)
{
    return basic_option<VALUE_T, DESCRIPTION_TYPE>(description);
}

} // namespace 

#endif
