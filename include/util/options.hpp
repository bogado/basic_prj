// options.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_OPTIONS_HPP
#define INCLUDED_OPTIONS_HPP

#include <concepts>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "util/string.hpp"
#include "util/converters.hpp"

namespace vb::opt {

template <typename OPTION_DESCRIPTION>
concept is_option_description = requires(const OPTION_DESCRIPTION value) {
    { value.key() } -> is_string;
    { value.abrev() } -> std::same_as<char>;
    { value.description() } -> is_string;
};

template <std::size_t SIZE>
struct opt_description {
    using position_type = std::size_t;

    static_string<SIZE> data;
    position_type start_of_description;

    constexpr opt_description(static_string<SIZE> dta)
        : data{dta}
        , start_of_description{data.view().find_first_of(':') + 1}
    {}

    constexpr auto key() const 
    {
        return data.view().substr(2);
    }

    constexpr auto abrev() const 
    {
        return data.view()[0];
    }

    constexpr auto description() const 
    {
        return data.view().substr(start_of_description);
    }

    constexpr bool operator== (is_string auto k)
    {
        return key() == k;
    }

    constexpr bool operator== (char abv)
    {
        return abrev() == abv;
    }
};

template <static_string OPT>
constexpr auto operator""_opt()
{
    return opt_description<decltype(OPT)::length>{OPT};
}

namespace test {
    static_assert(is_option_description<decltype("test|t:this is a test"_opt)>);
}

template <typename OPTION_T>
concept is_option = requires (const OPTION_T value) {
    typename OPTION_T::value_type;
    { OPTION_T::description() } -> is_option_description;
    { OPTION_T::parse(std::string("")) } -> std::same_as<OPTION_T>;
    { value.present() } -> std::convertible_to<bool>;
    { value->value() } -> std::same_as<typename OPTION_T::value_type>;
};

template <std::constructible_from<> VALUE_T>
auto default_builder() 
-> VALUE_T 
{ return {}; }

template<typename VALUE_T, static_string DESCRIPTION>
struct base_option {
    using value_type = VALUE_T;
    static constexpr auto option_description = opt_description{DESCRIPTION};

    static constexpr auto description() {
        return option_description;
    }

    base_option(value_type& original) :
        storage{original}
    {}

    std::reference_wrapper<value_type> storage;
    bool updated = false;

    constexpr auto parse(std::string_view str)
    {
        if constexpr (parse::parseable<value_type>) {
            return from_string<value_type>(str);
        }
    }

    auto value() const
        -> value_type
    {
        return storage.get();
    }

    bool present() const
    {
        return updated;
    }
};

namespace test {
    //constinit inline bool original = True;
    //constexpr auto test_option = "t|test:testing the options"_opt << original;
    //static_assert(is_option<base_option<bool, "t|test:testing">>);
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
