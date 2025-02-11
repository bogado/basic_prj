#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include "util/converters.hpp"
#include "util/string.hpp"

#include <concepts>
#include <iterator>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>
#include <string_view>
#include <cstdlib>
#include <optional>
#include <type_traits>

namespace vb {

using namespace std::literals;

namespace env {

struct variable;
struct environment;

struct variable_name {
    static constexpr auto SEPARATOR = '=';
    std::optional<std::string> storage;
    std::string_view my_name;

    constexpr auto data() const
    {
        return my_name.data();
    }

    friend struct variable;

    explicit variable_name(const variable& other);

public:
    consteval explicit variable_name(const char *var_name, std::size_t size) noexcept
    : my_name{var_name, size}
    {}

    explicit variable_name(std::string var_name)
    : storage{var_name}
    , my_name{storage.value()}
    {}


    explicit variable_name(variable& var) :
        variable_name{const_cast<const variable&>(var)} // NOLINT: cppcoreguidelines-pro-type-const-cast 
    {}

    constexpr std::size_t size() const noexcept
    {
        return my_name.size();
    }

    std::string to_string() const 
    {
        if (storage.has_value()) {
            return storage.value();
        } else {
           return std::string{my_name};
        }
    }

    explicit constexpr operator std::string_view() const noexcept {
        return my_name;
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

    template <parseable TYPE = std::string>
    std::optional<TYPE> value() const
    {
        auto opt_value = value_str();
        if (!opt_value.has_value()) {
            return {};
        }
        if constexpr (std::same_as<TYPE, std::string>) {
            return opt_value.value();
        } else {
            return { from_string<TYPE>(opt_value.value()) };
        }
    }

    template <parseable TYPE>
    auto value_or(TYPE default_val) const
    {
        return value<TYPE>().value_or(default_val);
    }
    
    constexpr auto operator <=>(const variable_name& other) const {
        return other.my_name <=> my_name;
    }

    constexpr bool operator==(const variable_name& other) const {
        return other.my_name == my_name;
    }

    constexpr bool operator!=(const variable_name& other) const = default;
};

struct variable {
    static constexpr auto SEPARATOR = '=';
private:
    std::string definition;
    std::optional<std::string_view> var_value;
    
    friend environment;

    auto split() const noexcept {
        auto pos = std::ranges::find(definition, SEPARATOR);
        return std::pair{std::string_view{definition.begin(), pos}, std::string_view{std::next(pos), definition.end()}}; 
    }

public:
    explicit variable(variable_name e_name) noexcept :
        definition{e_name.to_string() + SEPARATOR},
        var_value{}
    {}

    variable(is_string auto name) noexcept:
        definition(std::string(name) + SEPARATOR),
        var_value{}
    {}

    variable(is_string auto name, std::string_view val) noexcept:
        definition{to_string(name) + SEPARATOR + std::string(val)},
        var_value{val}
    {}

    variable_name name() const
    {
        return variable_name{*this};
    }

    template <typename VALUE_T = std::string_view>
    requires std::same_as<VALUE_T, std::string_view> || parseable<VALUE_T>
    auto value() const 
    {
        auto val = split().second;
        if constexpr (std::same_as<VALUE_T, std::string_view>) {
            return val;
        } else {
            return from_string<VALUE_T>(val);
        }
    }

    std::optional<std::string> value_str() const
    {
        auto val = value();
        if (val.empty()) {
            return {};
        } else {
            return std::string{val};
        }
    }

    bool is_sync() const
    {
        return name().value() == value();
    }

    void update_system() const {
        auto name_str = name().to_string();
        if (var_value.has_value()) {
            ::setenv(name_str.c_str(), std::string{var_value.value()}.c_str(), 1);
        } else {
            ::unsetenv(name_str.c_str());
        }
    }

    void update() {
    }

    auto definition_view() const
    {
        return std::string_view{definition};
    }

    auto to_string() const
        -> std::string
    {
        return definition;
    }

    friend bool operator==(const variable& me, const variable_name& name) {
        return me.name() == name;
    }

    friend bool operator==(const variable_name& name, const variable& me) {
        return me == name;
    }
};

variable_name::variable_name(const variable& var) :
    storage{},
    my_name(var.name())
{}

namespace literals {
    consteval auto operator""_env(const char* name, std::size_t size)
    {
        return variable_name{name, size};
    }
}

namespace test {
    using namespace literals;
    static_assert(static_cast<std::string_view>("HOME"_env) == "HOME");
}

struct environment {
    static constexpr auto SEPARATOR = '\0';
    static constexpr auto VALUE_SEPARATOR = '=';

private:
    static bool compare_names(const variable& a, const variable& b) {
        return a.name().to_string() < b.name().to_string();
    }

    std::vector<variable> definitions;
    std::vector<char *> environ;

    static auto grab_data(variable& var)
    {
        return var.definition.data();
    }

    void update()
    {
        std::ranges::sort(definitions, compare_names);
        environ.clear();
        for (auto& var: definitions) {
            environ.push_back(var.definition.data());
        }
    }

    auto add(variable var) {
        definitions.erase(std::ranges::find(definitions, var.name(), &variable::name));
        definitions.push_back(var);
    }

    struct mod {
        environment& self; // NOLINT: cppcoreguidelines-avoid-const-or-ref-data-members
        variable update;

        mod(environment& me, std::string_view n)
          : self{ me }
          , update{ n }
        {}

        void operator=(stringable auto value) // NOLINT: cppcoreguidelines-c-copy-assignment-signature
        {
            self.add(vb::to_string(value));
        }
    };

    auto lookup_name(std::string lookup) const
    {
        return std::ranges::find(definitions, variable_name{lookup}, &variable::name);
    }

public:
    environment() = default;
    explicit environment(const char **environment_)
       : definitions{}
    {
        while(environment_ != nullptr) {
            definitions.emplace_back(std::string{*environment_});
            environment_++;
        }
    }

    template <parseable RESULT_T>
    RESULT_T get(std::string name) const
    {
        auto it = lookup_name(name);
        if (it == std::end(definitions)) {
            return {};
        }
        return from_string<RESULT_T>(it->value());
    }

    bool contains(std::string name) 
    {
        return lookup_name(std::string{name}) != definitions.end();
    }
        
    auto set(std::string_view name) 
    {
        return mod{*this, name};
    }

    auto size() const
    {
        return definitions.size();
    }
};

}

namespace literals {
    using namespace env::literals;
}
}

#endif
