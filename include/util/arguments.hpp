#include <concepts>
#include <string.hpp>

namespace vb {

template <typename DESCRIPT_TYPE>
concept is_argument = requires(DESCRIPT_TYPE d) {
    std::constructible_from<const char*>;
};

}
