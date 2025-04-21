#ifndef INCLUDED_OPTIONAL_HPP
#define INCLUDED_OPTIONAL_HPP

#include <concepts>

namespace vb {

template <typename TYPE>
concept is_optional = requires(const TYPE value) {
    { value.has_value() } -> std::same_as<bool>;
    { value.value() } -> std::same_as<typename TYPE::value_type>;
};

}

#endif // INCLUDED_OPTIONAL_HPP

