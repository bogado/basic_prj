// options.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_OPTIONS_HPP
#define INCLUDED_OPTIONS_HPP

#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

#include <util/string.hpp>

namespace vb::options {

template <typename OPTION_DESCRIPTION>
concept is_option_description = requires(const OPTION_DESCRIPTION value) {
    { value.key() } -> is_string;
    { value.abrev() } -> std::same_as<char>;
    { value.description() } -> is_string;
};

struct opt_description {
    using position_type = std::string_view::size_type;

    std::string_view data;
    position_type start_of_description;

    constexpr opt_description(std::string_view dta)
        : data{dta}
        , start_of_description{data.find_first_of(":") + 1}
    {
        if (data[1] != '|' || start_of_description >= data.size())
        {
            throw "option syntax error";
        }
    }

    template <std::size_t SIZE>
    constexpr opt_description(const char (&dta)[SIZE])
        : opt_description{std::string_view{dta, SIZE}}
    {}

    constexpr auto key() const 
    {
        return data.substr(2);
    }

    constexpr auto abrev() const 
    {
        return data[0];
    }

    constexpr auto description() const 
    {
        return data.substr(start_of_description);
    }

    constexpr bool operator== (std::string k)
    {
        return key() == k;
    }

    constexpr bool operator== (char abv)
    {
        return abrev() == abv;
    }
};

constexpr auto operator""_opt(const char* option, std::size_t size)
{
    return opt_description{std::string_view{option, size}};
}

namespace test {
    static_assert(is_option_description<decltype("test|t:this is a test"_opt)>);
}

template <typename OPTION_T>
concept is_option = requires (const OPTION_T value) {
    typename OPTION_T::value_type;
    { OPTION_T::description() } -> is_option_description;
    { OPTION_T::parse(std::string("")) } -> std::same_as<OPTION_T>;
    { value.valid() } -> std::convertible_to<bool>;
    { value->value() } -> std::same_as<typename OPTION_T::value_type>;
};

template<typename VALUE_T, is_string auto DESCRIPTION>
struct base_option {
    using value_t = VALUE_T;
    static constexpr auto option_description = opt_description{DESCRIPTION};

    static constexpr auto description() {
        return option_description;
    }

    std::optional<value_t> content;

    auto value() const
        -> value_t
    { 
        return content.value_or({});
    }

    bool valid() const {
        return content.has_value();
    }
};

template <typename VALUE_T, static_string DESCRIPTION>
constexpr auto option(VALUE_T&& val)
{
    return base_option<VALUE_T, DESCRIPTION>{std::forward<VALUE_T>(val)};
}

namespace test {
    static_assert(is_option<base_option<bool, "t|test:testing"_opt>>);
    static_assert(is_option<decltype(option<bool, "t|test:testing"_opt>(true))>);
}

template <is_option ... OPTIONS>
struct options {
    using storage_type = std::tuple<OPTIONS...>;
    using value_type = std::variant<OPTIONS...>;
 
    storage_type storage;

    friend auto get(options opts, std::string_view key)
        -> value_type
    {
        constexpr auto index = [key]<std::size_t ... INDEXs>(std::index_sequence<INDEXs...>) {
            return ((std::tuple_element_t<INDEXs, storage_type>::key == key ? 0 : INDEXs) + ...);
        }(std::make_index_sequence<std::tuple_size_v<storage_type>>());

        return { std::get<index>(opts).value() };
    }
};


} // namespace BloombergLP

#endif
// --------------------------------------------------------------
// NOTICE:
// Copyright 2024 Bloomberg Finance L.P. All rights reserved.
// Property of Bloomberg Finance L.P. (BFLP)
// This software is made available solely pursuant to the
// terms of a BFLP license agreement which governs its use
// ----------------------- END-OF-FILE --------------------------
