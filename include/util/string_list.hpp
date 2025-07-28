#ifndef STRING_LIST_HPP_INCLUDED
#define STRING_LIST_HPP_INCLUDED

#include "./string.hpp"

#include <algorithm>
#include <iterator>
#include <ranges>

namespace vb {

using namespace std::literals;

template<std::size_t N>
using static_string_list = std::array<std::string_view, N>;

template<static_string STR, auto SEPARATOR = ',', std::size_t max = 0>
constexpr auto splited = []() {
    constexpr auto view = as_string_view(STR);

    auto result = [&]() {
        if constexpr (max == 0) {
            // "1,2,3" :
            //   - 2 separators
            //   - 3 items.
            return std::array<std::string_view, std::ranges::count(view, SEPARATOR) + 1>{};
        } else {
            return std::array<std::string_view, max>{};
        }
    }();

    auto strings = view | std::views::split(SEPARATOR) | std::views::transform([](const auto& range) {
                       return std::string_view(std::ranges::data(range), std::ranges::size(range));
                   });

    std::ranges::copy(strings | std::views::take(result.size()), std::begin(result));
    return result;
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

static constexpr auto splited_test_2 = splited<"test.1.2.3", '.', 2>;
static_assert(splited_test_2.size() == 2);
static_assert(splited_test_2[0] == "test");
static_assert(splited_test_2[1] == "1.2.3");
} // namespace test
}

#endif
