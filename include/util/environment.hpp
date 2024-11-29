#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include "util/converters.hpp"
#include "util/string.hpp"

#include <concepts>
#include <iterator>
#include <ranges>
#include <set>
#include <utility>
#include <vector>
#include <string_view>
#include <cstdlib>
#include <optional>
#include <type_traits>
#include <variant>

namespace vb {

using namespace std::literals;

namespace env {
struct variable;
struct environment;

struct variable_name {
    static constexpr auto SEPARATOR = '=';
private:
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

    std::set<variable, decltype(&compare_names)> definitions{compare_names};
    std::vector<char *> environ;

    static auto grab_data(variable& var)
    {
        return var.definition.data();
    }

    void update()
    {
        environ.clear();
        for (auto& var: definitions) {
            environ.push_back(var.definition.data());
        }
    }

    struct mod {
        environment& self; // NOLINT: cppcoreguidelines-avoid-const-or-ref-data-members
        std::string update;

        mod(environment& me, std::string_view n)
          : self{ me }
          , update{ n }
        {}

        void operator=(stringable auto value) // NOLINT: cppcoreguidelines-c-copy-assignment-signature
        {
            self.definitions += update + VALUE_SEPARATOR + to_string(value);
            self.definitions.insert(SEPARATOR);
        }
    };

    auto lookup_name(std::string lookup) const
    {
        lookup += VALUE_SEPARATOR;
        if (definitions.starts_with(lookup))
        {
            return std::next(std::begin(definitions), static_cast<int>(lookup.size() - 1));
        }

        lookup.insert(0, std::string{SEPARATOR});
        auto found = std::ranges::search(definitions, lookup);
        if (found.begin() == definitions.end()) {
            return std::end(definitions);
        }
        return std::prev(found.end());
    }

public:
    environment() = default;
    explicit environment(const char **environment_)
       : definitions{}
    {
        while(environment_ != nullptr) {
            definitions.insert(variable{std::string{*environment_}});
        }
    }

    template <parseable RESULT_T>
    RESULT_T get(std::string name) const
    {
        auto pos = lookup_name(name);
        return from_string<RESULT_T>(std::string_view(pos, std::find(pos, definitions.end(), SEPARATOR)));
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
        return std::ranges::count_if(definitions, [](auto c) { return c == SEPARATOR; });
    }
};

}

#endif
