#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include <string_view>
#include <cstdlib>
#include <array>
#include <optional>

#include "util/converters.hpp"

namespace vb {

using namespace std::literals;

struct env {
private:
    std::string_view var_name;
    std::optional<std::string_view> val;

public:
    constexpr env(std::string_view name_) noexcept :
        var_name(name_),
        val{std::nullopt}
    {}

    template <vb::parse::parseable TYPE>
    constexpr auto value(TYPE default_ = {}) const
    {
        if (!val.has_value()) {
            return default_;
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
