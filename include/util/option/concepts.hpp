#ifndef INCLUDED_CONCEPTS_HPP
#define INCLUDED_CONCEPTS_HPP

#include "../string.hpp"
#include "../optional.hpp"

#include <ranges>

namespace vb::opt {

template<typename OPTION_DESCRIPTION>
concept is_description = requires(const OPTION_DESCRIPTION value) {
    { value.key() } -> is_string;
    { value.shortkey() } -> std::same_as<char>;
    { value.description() } -> is_string;
    { value == 'a' } -> std::same_as<bool>;
    { value == std::string_view{} } -> std::same_as<bool>;
};

template<typename TYPE>
concept is_optional_description = is_optional<TYPE> && is_description<typename TYPE::value_type>;

template<typename PREFIX_T>
concept is_prefix_type = is_string<PREFIX_T> || std::same_as<PREFIX_T, char>;

template<typename ARGUMENT>
concept is_argument = is_string<ARGUMENT>;

template<typename ARGUMENT_LIST>
concept is_argument_list = std::ranges::range<ARGUMENT_LIST> && is_argument<typename ARGUMENT_LIST::value_type>;

template<typename OPTION_T>
concept is_valued = is_description<OPTION_T> && requires(const OPTION_T value) {
    typename OPTION_T::value_type;
    { (OPTION_T::has_default) } -> std::convertible_to<bool>;
    { value.parse("--option=value") } -> std::same_as<std::optional<typename OPTION_T::value_type>>;
};

template<typename PART>
concept is_string_part = is_string<PART> || std::same_as<PART, char>;

template<typename OPTION_GROUP>
concept is_group = std::integral<decltype(OPTION_GROUP::size)> && requires(const OPTION_GROUP group) {
    { group.name() } -> is_string;
    { group.description() } -> is_string;
    { group.template get<0>() } -> is_description;
    { group.locate(std::string_view{"option"}) } -> is_optional_description;
    { group.locate('a') } -> is_optional_description;
    { group[std::string_view{"--option"}] };
};
}


#endif // INCLUDED_CONCEPTS_HPP

