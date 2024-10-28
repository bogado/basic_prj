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
    std::optional<std::string> storage;
    std::string_view name;

    constexpr auto data() const
    {
        return name.data();
    }

public:
    consteval explicit env_name(const char *var_name, std::size_t size) noexcept
    : name{var_name, size}
    {}

    explicit env_name(std::string var_name)
    : storage{var_name}
    , name{storage.value()}
    {}

    constexpr std::size_t size() const noexcept
    {
        return name.size();
    }

    std::string to_string() const 
    {
        if (storage.has_value()) {
            return storage.value();
        } else {
           return std::string{name};
        }
    }

    explicit constexpr operator std::string_view() const noexcept {
        return name;
    }

    std::optional<std::string> value_str() const noexcept
    {
        auto var = to_string();
        if(auto value = ::getenv(var.c_str()); value != nullptr) {
            return std::string{value};
        } else {
            return {};
        }
    }

    template <vb::parse::parseable TYPE = std::string>
    std::optional<TYPE> value() const
    {
        auto opt_value = value_str();
        if (!opt_value.has_value()) {
            return {};
        }
        if constexpr (std::same_as<TYPE, std::string>) {
            return opt_value.value();
        } else {
            return { parse::from_string<TYPE>(opt_value.value()) };
        }
    }

    template <parse::parseable TYPE>
    auto value_or(TYPE default_val) const
    {
        return value<TYPE>().value_or(default_val);
    }
};

struct env {
    static constexpr auto SEPARATOR = '=';
private:
    std::string var_name;
    std::optional<std::string> var_value;

public:
    explicit env(env_name name) noexcept :
        var_name{name.to_string()},
        var_value{name.value_str()}
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

    auto to_string() const
        -> std::string
    {
        return std::string{name()} + SEPARATOR + var_value.value_or(std::string{});
    }
};

namespace literals {
    consteval auto operator""_env(const char* name, std::size_t size)
    {
        return env_name{name, size};
    }
}

namespace test {
    using namespace literals;
    static_assert("HOME"_env.name() == "HOME");
}

struct environment {
    std::vector<env> variables;
};

}

#endif
