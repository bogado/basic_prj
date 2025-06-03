#ifndef INCLUDED_OPTIONS_HPP
#define INCLUDED_OPTIONS_HPP

#include "./option/concepts.hpp"
#include "./option/description.hpp"
#include "./converters.hpp"
#include "./string.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string_view>
#include <tuple>
#include <utility>

namespace vb::opt {
using namespace std::literals;

static_assert(is_description<description::basic>);

template<std::constructible_from<> VALUE_T>
auto default_builder() -> VALUE_T
{
    return {};
}

template<typename VALUE_T, char SEPARATOR = '='>
struct basic_option : description::basic
{
    using base_value_type = VALUE_T;
    static constexpr auto separator = SEPARATOR;
    static constexpr auto has_default = std::same_as<bool, base_value_type>;
    using value_type = std::conditional_t<has_default, bool, std::optional<base_value_type>>;

    template<typename... ARG_Ts>
        requires(std::constructible_from<basic, ARG_Ts...>)
    explicit constexpr basic_option(ARG_Ts&&...args) noexcept
        : basic{ std::forward<ARG_Ts>(args)... }
    {
    }

    constexpr auto parse(std::string_view str) const -> value_type
    {
        if constexpr (std::same_as<bool, base_value_type>) {
            return *this == str;
        }
        if constexpr (parseable<base_value_type>) {
            if (*this == str) {
                auto value = str.substr(str.find(separator));
                if (!value.empty()) {
                    return from_string<base_value_type>(value);
                }
            }
        }
        return {};
    }
};

template<typename VALUE_T>
struct default_option : basic_option<VALUE_T>
{
    using super = basic_option<VALUE_T>;
    using base_value_type = VALUE_T;
    using value_type = VALUE_T;

    value_type my_default;

    static constexpr auto has_default = true;

    template<typename... ARG_Ts>
        requires(std::constructible_from<description::basic, ARG_Ts...>)
    explicit constexpr default_option(value_type default_value, ARG_Ts&&...args) noexcept
        : super{ std::forward<ARG_Ts>(args)... }
        , my_default{ default_value }
    {
    }

    constexpr auto parse(std::string_view str) const -> 
        value_type
    {
        if constexpr (super::has_default) {
            return super::parse(str) ? ! my_default: my_default;
        } else {
            auto opt = super::parse(str);
            if (opt.has_value()) {
                return opt;
            } else {
                return my_default;
            }
        }
    }
};

namespace test {
constexpr auto opt = basic_option<int>("--opt, -o: option <int>");

static_assert(is_valued<basic_option<int>>);
static_assert(is_valued<default_option<int>>);

};

template<typename VALUE_T, is_description DESCRIPTION_TYPE>
constexpr auto opt(DESCRIPTION_TYPE&& description)
{
    return basic_option<VALUE_T>(std::forward<DESCRIPTION_TYPE>(description));
}

} // namespace vb::opt

#endif
