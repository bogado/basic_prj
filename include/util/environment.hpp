#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include "util/converters.hpp"
#include "util/string.hpp"

#include <concepts>
#include <vector>
#include <string_view>
#include <cstdlib>
#include <optional>
#include <type_traits>
#include <variant>

namespace vb {

using namespace std::literals;

template <std::size_t SIZE>
struct env_name {
    static constexpr auto SEPARATOR = '=';
private:
    using string_type = std::conditional_t<SIZE == 0, std::string, static_string<SIZE>>;
    string_type name;

    constexpr auto data() const
    {
        return std::visit([](const auto& val) { return val.data(); }, name);
    }

public:
    constexpr explicit env_name(std::string_view name_) noexcept:
        name{std::string_view{name_.substr(0, name_.find(SEPARATOR))}}
    {}

    constexpr explicit env_name(const string_type name_) noexcept :
        env_name{std::string_view{name_}}
    {
    }

    constexpr std::size_t size() const noexcept
    {
        return std::visit([](const auto& val) { return val.size(); }, name);
    }

    std::string to_string() const {
        return std::string{data(), size()};
    }

    explicit constexpr operator std::string_view() const noexcept {
        return std::string_view{data(), size()};
    }

    std::optional<std::string> get_value() const noexcept
    {
        auto var = to_string();
        if(auto value = ::getenv(var.c_str()); value != nullptr) {
            return std::string{value};
        } else {
            return {};
        }
    }
};

struct env {
    static constexpr auto SEPARATOR = '=';
private:
    std::string var_name;
    std::optional<std::string> var_value;

public:
    template <std::size_t SIZE>
    explicit env(env_name<SIZE> name) noexcept :
        var_name{name.to_string()},
        var_value{name.get_value()}
    {}

    env(is_string auto name, std::string_view val) noexcept:
        var_name{name},
        var_value{val}
    {}

    constexpr std::string_view name() const
    {
        return static_cast<std::string_view>(var_name);
    }

    void update() const {
        auto name_str = var_name;
        if (var_value.has_value()) {
            ::setenv(name_str.c_str(), var_value.value().c_str(), 1);
        } else {
            ::unsetenv(name_str.c_str());
        }
    }

    template <vb::parse::parseable TYPE = std::string>
    std::optional<TYPE> value() const
    {
        if (var_value.has_value()) {
            return {};
        }
        if constexpr (std::same_as<TYPE, std::string>) {
            return var_value.value();
        } else {
            return { from_string<TYPE>(var_value.value()) };
        }
    }

    constexpr auto to_string() const
        -> std::string
    {
        return std::string{name()} + SEPARATOR + var_value.value_or(std::string{});
    }
};

namespace literals {
    template <char ... Cs>
    constexpr auto operator""_env()
        -> env_name<sizeof...(Cs)>
    {
        static auto data = std::array{Cs..., '\0'};
        return env_name<0>{std::string(data.data())};
    }
}

struct environment {
    std::vector<env> variables;
};

}

#endif
