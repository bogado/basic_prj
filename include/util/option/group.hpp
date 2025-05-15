#ifndef INCLUDED_GROUP_HPP
#define INCLUDED_GROUP_HPP

#include "./concepts.hpp"
#include "./description.hpp"

#include <optional>
#include <tuple>
#include <concepts>

namespace vb::opt {

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
        return result{};
    }
};

static_assert(is_group<option_group<basic_option<int>, basic_option<std::string_view>>>);
}

#endif // INCLUDED_GROUP_HPP

