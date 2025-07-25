#ifndef INCLUDE_UTIL_CONCEPT_HELPER_HPP
#define INCLUDE_UTIL_CONCEPT_HELPER_HPP

#include <concepts>
#include <type_traits>

namespace vb {

template <typename TYPE_A, typename TYPE_B>
concept static_same_as = std::same_as<std::remove_cvref_t<TYPE_A>, TYPE_B>;

}

#endif  // INCLUDE_UTIL_CONCEPT_HELPER_HPP
