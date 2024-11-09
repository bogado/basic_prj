#ifndef CONVERTERS_HPP_INCLUDED
#define CONVERTERS_HPP_INCLUDED

#include <util/string.hpp>
#include <iostream>
#include <sstream>
#include <string_view>
#include <string>

namespace vb {
namespace parse {

template <typename VALUE_T>
concept can_be_istreamed = requires(std::istream& in, VALUE_T val) {
    { in >> val } -> std::same_as<std::istream&>;
};

template <typename VALUE_T>
concept can_be_outstreamed = requires(std::ostream& out, VALUE_T val) {
    { out << val } -> std::same_as<std::ostream&>;
};

}

template <parse::can_be_outstreamed VALUE_T>
auto to_string(const VALUE_T& value)
-> std::string
{
    std::stringstream out{};
    out << value;
    return out.str();
}

template <parse::can_be_istreamed PARSEABLE>
constexpr auto from_string(is_string auto source) {
    auto result = PARSEABLE{};
    std::stringstream in{to_string(source)};
    in >> result;
    return result;
}

template <typename PARSEABLE>
concept parseable = requires (const std::string_view str) {
    { ::vb::from_string<PARSEABLE>(str) } -> std::same_as<PARSEABLE>;
};

template <typename STRINGABLE>
concept stringable = requires (const STRINGABLE value)
{
    { ::vb::to_string(value) } -> std::same_as<std::string>;
};

}

#endif
