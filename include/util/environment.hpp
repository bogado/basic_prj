#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include <cmath>
#include <string_view>
#include <cstdlib>
#include <optional>

#include "util/converters.hpp"

namespace vb {

using namespace std::literals;

struct env {
private:
    std::string_view var_name;
    std::optional<std::string_view> val;
    bool fetched = false;

    auto name_c()
    -> std::pair<const char *, std::string>
    {
        if (!var_name.empty() && var_name.back() == '\0') {
            return { var_name.data(), {} };
        }

        auto result = std::pair{
            "",
            std::string(var_name),
        };

        result.first = result.second.c_str();
        return result;
    }

public:
    constexpr explicit env(std::string_view name_) noexcept :
        var_name(name_),
        val{std::nullopt},
        fetched(false)
    {
        if (!std::is_constant_evaluated()) {
            init();
        }
    }

    constexpr void init()
    {
        if (fetched) return;
        auto data = std::getenv(name_c().first);
        if (data != nullptr) {
            val = data;
        }
        fetched = true;
    }

    template <vb::parse::parseable TYPE>
    auto value_or(TYPE default_ = {})
    {
        if (!fetched) {
            init();
        }

        if (!val.has_value() || val->empty()) {
            return default_;
        }

        return vb::parse::from_string<TYPE>(val.value());
    }

    template <vb::parse::parseable TYPE>
    auto value_or(TYPE default_ = {}) const
    -> TYPE
    {
        if (!fetched) {
            auto copy = *this;
            copy.init();
            return copy.value_or(default_);
        }
        return vb::parse::from_string<TYPE>(val.value());
    }

    constexpr auto name() const
    {
        return var_name;
    }

    constexpr auto to_string() const
    {
        return std::string(var_name) + "="s + std::string(val.value_or(""sv));
    }

};

namespace literals {

    constexpr auto operator""_env(const char* name, std::size_t len)
        -> env
    {
        return env(std::string_view(name, len+1));
    }
}

}

#endif
