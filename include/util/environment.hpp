#ifndef ENVIRONMENT_HPP_INCLUDED
#define ENVIRONMENT_HPP_INCLUDED

#include "util/converters.hpp"
#include "util/string.hpp"

#include <concepts>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <ranges>
#include <set>
#include <variant>
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
    using size_type = std::size_t;
private:
    std::variant<std::string, const char*> storage;
    size_type end_location{};

    constexpr std::string_view raw_view() const
    {
        return std::visit([&](const auto& value) {
            return std::string_view{value};
        }, storage);
    }

    constexpr auto data() const
    {
        return std::visit([]<typename TYPE>(const TYPE& value) {
            if constexpr (std::same_as<TYPE, std::string>){
                return value.data();
            } else {
                return value;
            }
        }, storage);
    }

    constexpr auto find_end() const 
    {
        auto raw = raw_view();
        return std::min(raw.size(), raw.find(SEPARATOR));
    }

    friend struct variable;

    explicit variable_name(const variable& other);

public:
    std::string raw_string() const 
    {
        return std::visit([](auto value) {
            return std::string(value);
        }, storage);
    }

    template <std::size_t N>
    consteval explicit variable_name(const char (&var_name)[N]) noexcept
    : storage{var_name}
    , end_location{find_end()}
    {
    }

    consteval explicit variable_name(const char *var_name, std::size_t) noexcept
    : storage{var_name}
    , end_location{find_end()}
    {}

    explicit variable_name(is_string auto var_name)
    : storage{std::string{var_name}}
    , end_location{find_end()}
    {}
    
    constexpr std::string_view name() const
    {
        return raw_view().substr(0, end_location);
    }

    std::string to_string() const noexcept
    {
        return std::string{name()};
    }

    std::optional<std::string> value_from_system() const noexcept
    {
        auto var = to_string();
        if(auto value = ::getenv(var.c_str()); value != nullptr) {
            return std::string{value};
        } else {
            return {};
        }
    }

    constexpr friend bool operator==(const variable_name& a, const variable_name& b)
    {
        return a.name() == b.name();
    }

    constexpr friend bool operator!=(const variable_name& a, const variable_name& b) = default;
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
      : definition{name.raw_string() + SEPARATOR + std::string(val)},
        var_value{definition.find(SEPARATOR)} {}

  explicit variable(variable_name name) noexcept
      : variable{name, name.value_from_system().value_or("")} {}

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

  const char* data() const {
      return definition.data();
  }

  void set(std::string new_value)
  {
    definition.replace(value_pos(), definition.size(), new_value);
  }

  template <parseable VALUE_T = std::string_view>
  auto value() const {
    return from_string<VALUE_T>(value_str());
  }

  bool has_value() {
      return definition.size() != value_pos();
  }

  std::string value_str() const
  {
      return definition.substr(value_pos());
  }

  bool is_sync() const
  {
      return name().value_from_system() == value_str();
  }

  void update_system() const {
      auto name_str = name().raw_string();
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

    static variable from_system(variable_name name) {
        if (auto value = name.value_from_system(); value.has_value()) {
            return variable{name, value.value()};
        } else {
            return variable{name};
        }
    }

    static variable from_system(is_string_class auto name) {
        return from_system(variable_name{name});
    }

    static variable from_system(const char* name) {
        return from_system(variable_name{name});
    }

    friend bool operator==(const variable& me, const variable_name& name) {
        return me.name() == name;
    }

    friend bool operator==(const variable_name& name, const variable& me) {
        return me == name;
    }
};

variable_name::variable_name(const variable& var)
    : storage{std::string{var.name().raw_view()}}
    , end_location{find_end()}
{}

namespace literals {
    consteval auto operator""_env(const char* name, std::size_t size)
    {
        return variable_name{name, size};
    }
}

namespace test {
    using namespace literals;
    static_assert("HOME"_env.name() == "HOME");
}

struct environment {
    static constexpr auto SEPARATOR = '\0';
    static constexpr auto VALUE_SEPARATOR = '=';

private:
    static bool compare_names(const variable& a, const variable& b) {
        return a.name().raw_string() < b.name().raw_string();
    }

    std::vector<variable> definitions{};
    std::vector<const char *> environ{1, nullptr};

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

    void update_environ()
    {
        std::size_t pos = 0;
        for (auto& definition : definitions) {
            if (environ[pos] == nullptr) {
                environ.push_back(nullptr);
            }
            environ[pos] = definition.data();
            pos++;
        }
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

    const char* const* getEnv() const
    {
        return environ.data();
    }

    void add(variable var)
    {
        if (auto old = std::ranges::find(definitions, var.name(), &variable::name); old != std::end(definitions)) {
            definitions.erase(old);
        }
        definitions.push_back(var);
        update_environ();
    }

    bool import(auto var)
    {
        variable definition = variable::from_system(var);
        add(definition);
        return definition.has_value();
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

    bool contains(std::string name) const
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
