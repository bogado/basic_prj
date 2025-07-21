#include <concepts>
#include <ranges>
#include <string>

#include "string.hpp"

namespace vb {

template <typename ARGUMENT_TYPE>
concept is_argument = std::constructible_from<ARGUMENT_TYPE, const char*>  && requires(const ARGUMENT_TYPE arg) {
    { arg.string() } -> std::same_as<std::string_view>;
    { arg.used() } -> std::convertible_to<bool>;
};


template <typename ARGUMENT_LIST>
concept is_argument_list = std::ranges::sized_range<ARGUMENT_LIST> &&
    is_argument<std::ranges::range_value_t<ARGUMENT_LIST>>;

struct argument
{
private:
    std::string_view my_value;

    argument(const char* arg) :
        my_value{arg}
    {}

public:
    std::string_view to_string() const {
        return my_value;
    }

    friend auto parse_arguments(std::size_t argc, const char* argv[])
    {
        return std::span{argv, argc} | std::views::transform([](auto v){ return argument{v}; });
    }
};


}
