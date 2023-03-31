#ifndef TYPESET_HPP_INCLUDED
#define TYPESET_HPP_INCLUDED

#include <concepts>
#include <limits>
#include <tuple>
#include <type_traits>
#include <optional>
#include <variant>
#include <algorithm>
#include <ranges>

namespace vb {

template <typename ... ELEMENTS>
struct type_set;

template <typename ... ELEMENTS>
struct type_multi_set {
    static constexpr auto size = sizeof...(ELEMENTS);

    template <typename T>
    static constexpr auto count_of = ((std::same_as<T, ELEMENTS>?1:0) + ...);

    template <typename T>
    static constexpr auto has = count_of<T> > 0;

    template <typename T>
    requires (count_of<T> > 0)
    static constexpr auto map = std::array<bool, sizeof...(ELEMENTS)> {std::is_same_v<T, ELEMENTS> ...};

    template <typename T>
    static constexpr auto index_of = []() {
        auto result = std::array<std::size_t, count_of<T>>{};
        if constexpr (result.size() == 0) {
            return result;
        } else {
            using namespace std::views;
            auto is_same_filter = [](auto indice) {
                return map<T>[indice];
            };
            std::ranges::copy(std::views::iota(std::size_t{0},size) | std::views::filter(is_same_filter), std::begin(result));
            return result;
        }
    }();
};

namespace static_test {
    using multi_set_ = type_multi_set<char, int, char, double, double>;

    static_assert(multi_set_::count_of<char> == 2);
    static_assert(sizeof(multi_set_) == 1);
    static_assert(multi_set_::index_of<char>[0] == 0);
}

template <>
struct type_set<>
{
    static constexpr auto not_present = std::numeric_limits<std::size_t>::max();
    template <typename OTHER>
    static constexpr bool has = false;

    static constexpr auto size  = 0;
    static constexpr auto empty = true;
    static constexpr auto valid = true;

    using head_type = void;
    using tail_type = type_set;
    using canonical = type_set;

};

template <typename FIRST, typename... ELEMENTS>
struct type_set<FIRST, ELEMENTS...>
{
    static constexpr auto not_present = type_set<>::not_present;
    static constexpr auto size = sizeof...(ELEMENTS);
    static constexpr auto valid = (!std::same_as<FIRST, ELEMENTS> && ...) && type_set<ELEMENTS...>::valid;

    using head_type = FIRST;
    using tail_type_set = type_set<ELEMENTS...>;
    using as_tuple = std::tuple<FIRST, ELEMENTS...>;
    using as_variant = std::tuple<FIRST, ELEMENTS...>;
    using as_optional = std::conditional<size == 1, std::optional<FIRST>, std::variant<std::monostate, FIRST, ELEMENTS...>>;

    template <typename T>
    static constexpr bool has = std::same_as<T, FIRST> || (std::is_same_v<T, ELEMENTS> || ...);

    template <typename ELEMENT>
    using insert = std::conditional_t<has<ELEMENT>, type_set, type_set<ELEMENTS..., ELEMENT>>;

    template <typename T>
    static constexpr auto index = []<std::size_t... INDICES>(std::index_sequence<INDICES...>)
    {
        auto index = ((std::same_as<T, std::tuple_element_t<INDICES, as_tuple>>?(INDICES+1):0) + ...);

        return index > 0 ? index -1: not_present;
    } (std::make_index_sequence<sizeof...(ELEMENTS)+1>());
};

template <typename TYPESET>
concept is_type_set = sizeof(std::pair<TYPESET,char>) == 1 &&
    requires {
        typename TYPESET::head_type;
        typename TYPESET::tail_type;
        { TYPESET::size } -> std::integral;
    }
    && TYPESET::template has<typename TYPESET::head_type>;

namespace static_test {
static_assert(!type_set<char, double>::has<float>);
static_assert(type_set<char, double>::index<char> == 0);
static_assert(type_set<char, double>::index<double> == 1);
static_assert(type_set<char, double>::has<double>);
static_assert(type_set<char, double>::has<char>);

}

}
#endif
