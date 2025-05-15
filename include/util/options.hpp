#ifndef INCLUDED_OPTIONS_HPP
#define INCLUDED_OPTIONS_HPP

#include "./option/description.hpp"
#include "./converters.hpp"
#include "./string.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string_view>
#include <tuple>
#include <utility>

namespace vb::opt {
using namespace std::literals;

template<typename ARGUMENT>
concept is_argument = is_string<ARGUMENT>;

template<typename ARGUMENT_LIST>
concept is_argument_list = std::ranges::range<ARGUMENT_LIST> && is_argument<typename ARGUMENT_LIST::value_type>;

template<typename OPTION_GROUP>
concept is_group = std::integral<decltype(OPTION_GROUP::size)> && requires(const OPTION_GROUP group) {
    { group.name() } -> is_string;
    { group.description() } -> is_string;
    { group.template get<0>() } -> is_description;
    { group.locate("option"sv) };
    { group.locate('a') };
    { group["--option"sv] };
    { group.parse(std::span<std::string_view>{}) } -> std::same_as<typename OPTION_GROUP::result_type>;
};

template<typename OPTION_T>
concept is_valued = is_description<OPTION_T> && requires(const OPTION_T value) {
    typename OPTION_T::value_type;
    { (OPTION_T::has_default) } -> std::convertible_to<bool>;
    { value.parse("--option=value") } -> std::same_as<std::optional<typename OPTION_T::value_type>>;
};

static_assert(is_description<basic_description>);

template<std::constructible_from<> VALUE_T>
auto default_builder() -> VALUE_T
{
    return {};
}

template<typename VALUE_T>
struct basic_option : basic_description
{
    using value_type = VALUE_T;

    static constexpr auto has_default = false;

    template<typename... ARG_Ts>
        requires(std::constructible_from<basic_description, ARG_Ts...>)
    explicit constexpr basic_option(ARG_Ts&&...args) noexcept
        : basic_description{ std::forward<ARG_Ts>(args)... }
    {
    }

    bool is_updated = false;

    constexpr static auto parse(std::string_view str) -> std::optional<value_type>
    {
        if constexpr (parseable<value_type>) {
            return from_string<value_type>(str);
        }
    }
};

template<typename VALUE_T>
struct default_option : basic_option<VALUE_T>
{
    using value_type = basic_option<VALUE_T>::value_type;
    value_type default_value;

    static constexpr auto has_default = true;

    template<typename... ARG_Ts>
        requires(std::constructible_from<basic_description, ARG_Ts...>)
    explicit constexpr basic_option(ARG_Ts&&...args) noexcept
        : basic_description{ std::forward<ARG_Ts>(args)... }
    {
    }

    bool is_updated = false;

    constexpr static auto parse(std::string_view str) -> std::optional<value_type>
    {
        if constexpr (parseable<value_type>) {
            return from_string<value_type>(str);
        }
    }
};
namespace test {
constexpr auto opt = basic_option<int>("--opt, -o: option <int>");

static_assert(is_valued<basic_option<int>>);

};

template<is_valued... OPTION_Ts>
class option_group
{
public:
    using storage_type = std::tuple<OPTION_Ts...>;
    using option_type  = std::optional<basic_description>;
    using result_type  = std::tuple<typename OPTION_Ts::value_type...>;

    constexpr static auto size = sizeof...(OPTION_Ts);

private:
    static constexpr auto SEPARATOR = ':';

    static constexpr auto indexes = std::make_index_sequence<size>();

    storage_type     my_options;
    std::string_view my_name;
    std::string_view my_description;

    template<bool IS_NAME>
    static constexpr std::string_view find_part(const std::string_view def)
    {
        auto loc = def.find(SEPARATOR);
        if (loc == def.npos) {
            return std::string_view{};
        }
        if constexpr (IS_NAME) {
            return def.substr(0, def.find(SEPARATOR));
        } else {
            return def.substr(def.find(SEPARATOR) + 1);
        }
    }

public:
    template<std::size_t DEF_SIZE>
    explicit consteval option_group(
        const char (&def)[DEF_SIZE], // NOLINT[cppcoreguidelines-avoid-c-arrays]
        OPTION_Ts&&...options)
        : my_options{ options... }
        , my_name{ find_part<true>(def) }
        , my_description{ find_part<false>(def) }
    {
    }

    template<typename Self>
    auto begin(this Self&& self)
    {
        return std::forward<Self>(self).my_options.begin();
    }

    template<typename Self>
    auto end(this Self&& self)
    {
        return std::forward<Self>(self).my_options.end();
    }

    constexpr auto name() const { return my_name; }

    constexpr auto description() const { return my_description; }

    template<std::size_t POS>
    constexpr auto get() const
    {
        return std::get<POS>(my_options);
    }

    constexpr auto locate() const {}

private:
    template<bool SELF, typename OTHER>
    static bool compare(const is_description auto& description, const OTHER& other)
    {
        if constexpr (SELF) {
            return description == other;
        } else if constexpr (std::same_as<OTHER, char>) {
            return description.shortkey() == other;
        } else {
            return description.key() == as_string_view(other);
        }
    }

    template<is_string_part T, std::size_t POS, bool SELF>
    constexpr auto getter(const T& value) const -> option_type
    {
        if constexpr (POS >= size) {
            return option_type{};
        } else {
            if (const auto& current = get<POS>(); compare<SELF>(current, value)) {
                return option_type{ current };
            } else {
                return getter<T, POS + 1, SELF>(value);
            }
        }
    }

public:
    template<typename T>
        requires(std::same_as<T, std::string_view> || std::same_as<T, char>)
    constexpr auto operator[](T value) const
    {
        return getter<T, 0, true>(value);
    }

    template<typename T, std::size_t POS = 0>
        requires(std::same_as<T, std::string_view> || std::same_as<T, char>)
    constexpr auto locate(T value) const -> option_type
    {
        return getter<T, 0, false>(value);
    }

    constexpr auto parse(is_argument_list auto args) {
        result_type result;
        for 
    }
};

static_assert(is_group<option_group<basic_option<int>, basic_option<std::string_view>>>);

template<typename VALUE_T, is_description DESCRIPTION_TYPE>
constexpr auto opt(DESCRIPTION_TYPE&& description)
{
    return basic_option<VALUE_T>(std::forward<DESCRIPTION_TYPE>(description));
}

} // namespace vb::opt

#endif
