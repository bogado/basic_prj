#ifndef CONVERTERS_HPP_INCLUDED
#define CONVERTERS_HPP_INCLUDED

#include "string.hpp"

#include <iostream>
#include <sstream>
#include <string_view>

namespace vb::parse {

template <typename VALUE_T>
concept can_be_streamed = requires(std::istream& in, VALUE_T val) {
    { in >> val } -> std::same_as<std::istream&>;
};

template <typename PARSEABLE>
auto from_string(std::string_view source) {
    if constexpr (can_be_streamed<PARSEABLE>) {
        auto result = PARSEABLE{};
        std::stringstream in{std::string{source}};
        in >> result;
        return result;
    } else {
        throw "Not parseable";
    }
}

template <typename PARSEABLE>
concept parseable = requires (const std::string_view str) {
    { from_string<PARSEABLE>(str) } -> std::same_as<PARSEABLE>;
};

}
#endif
