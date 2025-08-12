#ifndef STRING_LIST_HPP_INCLUDED
#define STRING_LIST_HPP_INCLUDED

#include "./string.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <iterator>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace vb {

using namespace std::literals;

static constexpr inline auto NO_MAXIMUM             = std::numeric_limits<std::size_t>::max();
static constexpr inline auto DEFAULT_LIST_SEPARATOR = ',';

constexpr auto count_parts(
    const is_string auto& source,
    char                  separator = DEFAULT_LIST_SEPARATOR,
    std::size_t           max_count = NO_MAXIMUM) -> std::size_t
{
    return std::min(static_cast<std::size_t>(std::ranges::count(as_string_view(source), separator) + 1), max_count);
}

template<std::size_t MAX = NO_MAXIMUM>
constexpr auto split_string(const is_string auto& source, char separator = DEFAULT_LIST_SEPARATOR)
{
    auto       view   = as_string_view(source);
    const auto count  = count_parts(source, separator, MAX);
    auto       result = [&]() {
        if constexpr (MAX == NO_MAXIMUM) {
            return std::vector<std::string_view>{ count, std::string_view{} };
        } else {
            return std::array<std::string_view, MAX>{};
        }
    }();

    for (auto [index, ret] : std::ranges::iota_view(std::size_t{ 0 }, count) |
                                 std::views::transform([&, it = std::begin(view)](const auto& index) mutable {
                                     if (index == count - 1) {
                                         return std::pair{ index, std::string_view(it, std::end(view)) };
                                     }

                                     auto start = it;
                                     auto end   = std::ranges::find(start, std::end(view), separator);
                                     if (end != std::end(view)) {
                                         it = std::next(end, 1);
                                     } else {
                                         it = end;
                                     }
                                     return std::pair{ index, std::string_view(start, end) };
                                 })) {
        result.at(index) = ret;
    }
    return result;
}

template<std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template<static_string STR, auto SEPARATOR = ',', std::size_t MAX = NO_MAXIMUM>
constexpr auto splited = []() {
    constexpr auto view  = as_string_view(STR);
    constexpr auto count = count_parts(view, SEPARATOR, MAX);
    static_assert(count != NO_MAXIMUM);
    static_assert(count > 0);

    return split_string<count>(view, SEPARATOR);
}();

namespace test {
using namespace std::literals;
static constexpr auto splited_test = splited<"test,a,b,c">;
static constexpr auto first        = splited_test[0];

static_assert(std::same_as<decltype(first), const std::string_view>);
static_assert(std::size(splited_test) == 4);
static_assert(first == "test"sv);
static_assert(splited_test[1] == "a"sv);
static_assert(splited_test[2] == "b"sv);
static_assert(splited_test[3] == "c"sv);

static_assert(count_parts("1,2,3,4") == 4);
static_assert(count_parts("1.2.3.4") == 1);
static_assert(count_parts("1.2.3.4", '.') == 4);
static_assert(count_parts("1.2.3.4", '.', 2) == 2);
static_assert(count_parts("1.2.3.4", ',', 2) == 1);

static constexpr auto splited_test_2 = splited<"test.1.2.3", '.', 2>;
static_assert(splited_test_2.size() == 2);
static_assert(splited_test_2[0] == "test");
static_assert(splited_test_2[1] == "1.2.3");
} // namespace test

}

#endif
