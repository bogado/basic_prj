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

struct env_name {
    static constexpr auto SEPARATOR = '=';
private:
    std::variant<std::string, std::string_view> name;

    constexpr auto data() const
    {
        return std::visit([](const auto& val) { return val.data(); }, name);
    }

public:
    constexpr explicit env_name(std::string_view name_) noexcept:
        name{std::string_view{name_.substr(0, name_.find(SEPARATOR))}}
    {}

    template <std::size_t SIZE>
    consteval explicit env_name(const char name_[SIZE]) noexcept :
        env_name{std::string_view{name_, SIZE}}
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
    env_name var_name;
    std::optional<std::string> var_value;

public:
    explicit env(env_name name) noexcept :
        var_name{name},
        var_value{name.get_value()}
    {}

    env(env_name name, std::string_view val) noexcept:
        var_name{name},
        var_value{val}
    {}

    constexpr std::string_view name() const
    {
        return static_cast<std::string_view>(var_name);
    }

    void update() const {
        auto name_str = var_name.to_string();
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
        return var_name.to_string() + SEPARATOR + var_value.value_or(std::string{});
    }
};

namespace literals {
    constexpr auto operator""_env(const char* name, std::size_t len)
        -> env_name
    {
        return env_name(std::string(name, len));
    }
}

struct environment {
    std::vector<env> variables;
};

}

#endif
