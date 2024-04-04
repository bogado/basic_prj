#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include <cmath>
#include <string_view>
#include <cstdlib>
#include <optional>

#include "util/converters.hpp"
#include "util/string.hpp"

namespace vb {

using namespace std::literals;

struct env {
private:
    std::string_view variable_name;
    std::string_view value;
    std::string_view content;
    bool fetched = false;

    static constexpr auto equal_pos(const is_string auto& content)
    {
        return std::string_view(content).find_first_of('=');
    }

    static constexpr auto value_pos(const is_string auto& content)
    {
        auto pos = equal_pos(content);
        if (pos == std::string_view::npos) {
            return content.size();
        }

        return std::min(content.size(), pos +1);
    }

public:
    constexpr explicit env(std::string_view name_or_content) noexcept :
        variable_name(name_or_content.substr(0, equal_pos(name_or_content))),
        value(name_or_content.substr(value_pos(name_or_content))),
        content(value.empty() ? value : name_or_content),
        fetched(!value.empty())
    {
        if (!fetched && !std::is_constant_evaluated()) {
            init();
        }
    }

    template <std::size_t SIZE>
    constexpr explicit env(static_string<SIZE> name_or_content) noexcept :
        env{name_or_content}
    {}

    constexpr void init()
    {
        if (fetched) return;
        auto var_name = std::string{variable_name};
        auto data = std::getenv(var_name.c_str());
        if (data != nullptr) {
            value = data;
        } else {
            value = std::string_view();
        }
        fetched = true;
    }

    template <vb::parse::parseable TYPE>
    auto value_or(TYPE default_ = {})
    {
        if (!fetched) {
            init();
        }

        if (value.empty()) {
            return default_;
        }

        return vb::parse::from_string<TYPE>(value);
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
        return vb::parse::from_string<TYPE>(value);
    }

    constexpr auto name() const
    {
        return variable_name;
    }

    constexpr auto to_string() const
        -> std::string
    {
        auto result = std::string{content};
        if (result.empty()) 
        {
            result.append(variable_name);
            result.append("=");
            result.append(value_or<std::string>());
        }
        return result;
    }

};

namespace literals {

    constexpr auto operator""_env(const char* name, std::size_t len)
        -> env
    {
        return env(std::string_view(name, len));
    }
}

}

#endif
