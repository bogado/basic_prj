#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include "util/converters.hpp"
#include "util/string.hpp"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <set>
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
    template <std::size_t N>
    consteval explicit variable_name(const char (&var_name)[N]) noexcept
    : my_name{var_name, N}
    {}

    consteval explicit variable_name(const char *var_name, std::size_t size) noexcept
    : my_name{var_name, size}
    {}

    explicit variable_name(is_string auto var_name)
    : storage{std::string{std::begin(var_name), std::ranges::find(var_name, SEPARATOR)}}
    , my_name{storage.value()}
    {}

    explicit variable_name(variable& var) :
        variable_name{const_cast<const variable&>(var)} // NOLINT: cppcoreguidelines-pro-type-const-cast 
    {}

    constexpr std::size_t size() const noexcept
    {
        return my_name.size();
    }

    constexpr std::ptrdiff_t ssize() const noexcept
    {
        return static_cast<std::ptrdiff_t>(my_name.size());
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
    std::string::size_type var_value;
    
    friend environment;

    constexpr auto value_pos() const 
    {
        return var_value + 1;
    }

public:
  variable(variable_name name, std::string_view val) noexcept
      : definition{name.to_string() + SEPARATOR + std::string(val)},
        var_value{definition.find(SEPARATOR)} {}

  explicit variable(variable_name name) noexcept
      : variable{name, name.value_str().value_or("")} {}

  explicit variable(std::string_view def) noexcept
      :  definition{def},
        var_value{definition.find(SEPARATOR)}
  {
      if (var_value == std::string::npos) {
          var_value = definition.size();
          definition.push_back(SEPARATOR);
      }
  }

  variable_name name() const {
      return variable_name{definition};
  }

  void set(std::string new_value)
  {
    definition.replace(value_pos(), definition.size(), new_value);
  }

  template <parseable VALUE_T = std::string_view>
  auto value() const {
    return from_string<VALUE_T>(value_str());
  }

  std::string value_str() const
  {
      return definition.substr(value_pos());
  }

  bool is_sync() const
  {
      return name().value() == value_str();
  }

  void update_system() const {
      auto name_str = name().to_string();
      if (var_value < definition.size()) {
          ::setenv(name_str.c_str(), value_str().c_str(), 1);
      } else {
          ::unsetenv(name_str.c_str());
      }
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

    struct mod {
        environment& self; // NOLINT: cppcoreguidelines-avoid-const-or-ref-data-members
        variable update;

        mod(environment& me, std::string_view name)
          : self{ me }
          , update{ variable_name{name} }
        {}

        void operator=(stringable auto value) // NOLINT: cppcoreguidelines-c-copy-assignment-signature
        {
            update.set(vb::to_string(value));
            self.add(update);
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
            definitions.emplace_back(variable{*environment_});
            environment_++;
        }
    }

    char* const* getEnv() const
    {
        return environ.data();
    }

    void add(variable var)
    {
        if (auto old = std::ranges::find(definitions, var.name(), &variable::name); old != std::end(definitions)) {
            definitions.erase(old);
        }
        definitions.push_back(var);
    }

    template <parseable RESULT_T>
    RESULT_T get(std::string name) const
    {
        auto it = lookup_name(name);
        if (it == std::end(definitions)) {
            return {};
        }
        return it->value<RESULT_T>();
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
