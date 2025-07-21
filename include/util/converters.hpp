#ifndef CONVERTERS_HPP_INCLUDED
#define CONVERTERS_HPP_INCLUDED

#include "./string.hpp"

#include <concepts>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace vb {

template<typename VALUE_T>
concept can_be_istreamed = requires(std::istream& in, VALUE_T val) {
    { in >> val } -> std::same_as<std::istream&>;
};

template<typename VALUE_T>
concept can_be_outstreamed = requires(std::ostream& out, VALUE_T val) {
    { out << val } -> std::same_as<std::ostream&>;
};

template<can_be_outstreamed VALUE_T>
auto to_string(const VALUE_T& value) -> std::string
{
    if constexpr (std::convertible_to<std::string, VALUE_T>) {
        return static_cast<std::string>(value);
    } else if constexpr (std::constructible_from<std::string, VALUE_T>) {
        return std::string{ value };
    }
    std::stringstream out{};
    out << value;
    return out.str();
}

template<can_be_istreamed PARSEABLE>
auto from_string(std::string_view source) -> PARSEABLE
{
    if constexpr (std::same_as<std::string_view, PARSEABLE> || std::same_as<std::string, PARSEABLE>) {
        return PARSEABLE{ source };
    }
    auto              result = PARSEABLE{};
    std::stringstream in{ to_string(source) };
    in >> result;
    return result;
}

template<can_be_istreamed PARSEABLE>
constexpr auto from_string(is_string auto source)
{
    return from_string<PARSEABLE>(as_string_view(source));
}

template<typename PARSEABLE>
concept parseable = requires(const std::string_view str) {
    { vb::from_string<PARSEABLE>(str) } -> std::same_as<PARSEABLE>;
};

template<typename STRINGABLE>
concept stringable = requires(const STRINGABLE value) {
    { ::vb::to_string(value) } -> std::same_as<std::string>;
};

template<typename FORMATABLE>
concept formatable = stringable<FORMATABLE> && std::is_class_v<FORMATABLE>;

namespace static_test {
static_assert(parseable<int>);
static_assert(stringable<int>);

static_assert(parseable<double>);
static_assert(stringable<double>);

static_assert(parseable<std::string>);
static_assert(stringable<std::string>);

static_assert(!parseable<std::string_view>);
static_assert(stringable<std::string_view>);
}

}

template<vb::formatable TYPE>
struct std::formatter<TYPE, char>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(const TYPE& value, FORMAT_CONTEXT& context) const
    {
        auto out = std::ranges::copy(vb::to_string(value), context.out()).out;

        return out;
    }
};

#endif
