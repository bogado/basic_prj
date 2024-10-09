#ifndef EXPECTED_HPP_INCLUDED
#define EXPECTED_HPP_INCLUDED

#include <concepts>
#include <exception>
#include <format>
#if __cpp_lib_expected > 202211L

#include <expected>

#else

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace vb::st {

// class template unexpected
template<class UNEXPECTED_T>
class unexpected
{
  public:
    // constructors
    template<class... Args>
        requires std::constructible_from<UNEXPECTED_T, Args...>
    constexpr explicit unexpected(std::in_place_t, Args&&...args)
      : unex{ std::forward<Args>(args)... }
    {
    }

    template<class U, class... Args>
    constexpr explicit unexpected(std::in_place_t,
                                  std::initializer_list<U> init,
                                  Args&&...args)
      : unex{ init, std::forward<Args>(args)... }
    {
    }

    template<class Err = UNEXPECTED_T>
        requires std::constructible_from<UNEXPECTED_T, Err>
    constexpr explicit unexpected(Err&& error)
      : unex{ error }
    {
    }

    // observer
    template<typename T>
    constexpr auto error(this T&& self) noexcept
    {
        return std::forward<T>(self.unex);
    }

    // swap
    constexpr void swap(unexpected& other) noexcept
    {
        std::swap(unex, other.unex);
    }

    friend constexpr void swap(unexpected& x,
                               unexpected& y) noexcept(noexcept(x.swap(y)))
    {
        std::swap(x.unex, y.unex);
    }

    // equality operator
    template<class E2>
        requires std::equality_comparable_with<UNEXPECTED_T, E2>
    friend constexpr bool operator==(const unexpected& value,
                                     const unexpected<E2>& value2)
    {
        return value.unex == value2.unex;
    }

  private:
    UNEXPECTED_T unex; // exposition only
};

// class template bad_expected_access
template<class ERROR_T>
class bad_expected_access;

// specialization of bad_expected_access for void
template<>
class bad_expected_access<void> : public std::exception
{};

template<class ERROR_T>
class bad_expected_access : public bad_expected_access<void>
{
    ERROR_T error_obj;

  public:
    bad_expected_access(ERROR_T err)
      : error_obj(std::move(err))
    {
    }

    const char *what() const noexcept override
    {
        return "Bad bad_expected_access";
    }

    template<typename T>
    constexpr auto error(this T&& self) noexcept
    {
        return std::forward<T>(self.error_obj);
    }
};

// in-place construction of unexpected values
struct unexpect_t
{
    explicit unexpect_t() = default;
};

inline constexpr unexpect_t unexpect{};

// class template expected
template<class EXPECTED_T, class UNEXPECTED_T>
class expected
{
  public:
    using value_type = EXPECTED_T;
    using error_type = UNEXPECTED_T;
    using unexpected_type = unexpected<UNEXPECTED_T>;

    template<class REBOUND_TYPE>
    using rebind = expected<REBOUND_TYPE, error_type>;

    // constructors
    constexpr expected();
    template<typename EXPECTED_LIKE_T>
        requires std::convertible_to<EXPECTED_LIKE_T, value_type>
    expected(EXPECTED_LIKE_T value)
      : data{ value }
    {
    }

    template<class G>
        requires std::convertible_to<G, error_type>
    constexpr expected(const unexpected<G>& unexpected_obj)
      : data{ unexpected_obj.error }
    {
    }

    template<class G>
        requires std::convertible_to<G, error_type>
    constexpr expected(const unexpected<G>&& unexpected_obj)
      : data{ std::move(unexpected_obj.error) }
    {
    }

    template<class... Args>
        requires std::constructible_from<value_type, Args...>
    constexpr explicit expected(std::in_place_t, Args&&...args)
      : data{ value_type{ std::forward<Args>(args)... } }
    {
    }

    template<class U, class... Args>
        requires std::
          constructible_from<value_type, std::initializer_list<U>, Args...>
      constexpr explicit expected(std::in_place_t,
                                  std::initializer_list<U> init,
                                  Args&&...args)
      : data{ value_type{ init, std::forward<Args>(args)... } }
    {
    }

    template<class... Args>
        requires std::constructible_from<error_type, Args...>
    constexpr explicit expected(unexpect_t, Args&&...args)
      : data{ error_type{ std::forward<Args>(args)... } }
    {
    }

    template<class U, class... Args>
        requires std::
          constructible_from<error_type, std::initializer_list<U>, Args...>
      constexpr explicit expected(unexpect_t,
                                  std::initializer_list<U> init,
                                  Args&&...args)
      : data{ error_type{ init, std::forward<Args>(args)... } }
    {
    }

    template<class... Args>
        requires std::constructible_from<value_type, Args...>
    constexpr EXPECTED_T& emplace(Args&&...args) noexcept
    {
        data = value_type{ std::forward<Args>(args)... };
    }

    template<class U, class... Args>
        requires std::
          constructible_from<value_type, std::initializer_list<U>, Args...>
      constexpr EXPECTED_T& emplace(std::initializer_list<U> init,
                                    Args&&...args) noexcept
    {
        data = error_type{ init, std::forward<Args>(args)... };
    }

    template<typename T>
    constexpr const error_type& error(this T&& self)
    {
        if (self) {
            throw bad_expected_access<void>{};
        }
        return std::get<1>(std::forward<T>(self).data);
    }

    constexpr UNEXPECTED_T& error() &;

    template<typename T>
    constexpr auto& value(this T&& self)
    {
        if (!self) {
            throw bad_expected_access{ self.error() };
        }
        return std::get<0>(std::forward<T>(self).data);
    }

    // observers
    template<typename T>
    constexpr auto *operator->(this T&& self) noexcept
    {
        return &std::get<0>(std::forward<T>(self));
    }

    template<typename T>
    constexpr auto operator*(this T&& self) noexcept
    {
        return &std::get<0>(std::forward<T>(self));
    }

    constexpr bool has_value() const noexcept
    {
        return std::holds_alternative<0>(data);
    }

    constexpr explicit operator bool() const noexcept { return has_value(); }

    template<typename SELF_T, typename U>
        requires std::convertible_to<U, value_type>
    constexpr EXPECTED_T value_or(this SELF_T&& self, U&& other)
    {
        if (!self.has_value()) {
            return std::forward<other>(other);
        }
        return std::forward<SELF_T>(self).value();
    }

    template<typename SELF_T, class G = UNEXPECTED_T>
    constexpr UNEXPECTED_T error_or(this SELF_T&& self, G&& error)
    {
        if (self.has_value()) {
            return std::forward<G>(error);
        }
        return std::forward<SELF_T>(self).error();
    }

    // monadic operations
    template<class F>
        requires std::invocable<F>
    constexpr auto and_then(F&& f)
    {
        if (has_value()) {
            return expected{ std::forward(f)() };
        }
    }

    template<typename SELF_T, class F>
        requires std::invocable<F, value_type>
    constexpr auto and_then(this SELF_T&& s, F&& f)
    {
        auto self = std::forward<SELF_T>(s);
        if (self.has_value()) {
            return std::forward(f)(std::forward(self).value());
        }
    }

    template<typename SELF_T, class F>
        requires std::invocable<F>
    constexpr auto or_else(this SELF_T&& s, F&& f)
    {
        auto self = std::forward<SELF_T>(s);
        if (!self.has_value()) {
            return std::forward<F>(f)();
        }
    }
    constexpr auto transform(F&& f) &;
    template<class F>
    constexpr auto transform(F&& f) &&;
    template<class F>
    constexpr auto transform(F&& f) const&;
    template<class F>
    constexpr auto transform(F&& f) const&&;
    template<class F>
    constexpr auto transform_error(F&& f) &;
    template<class F>
    constexpr auto transform_error(F&& f) &&;
    template<class F>
    constexpr auto transform_error(F&& f) const&;
    template<class F>
    constexpr auto transform_error(F&& f) const&&;

    // equality operators
    template<class T2, class E2>
        requires(!std::is_void_v<T2> &&
                 std::equality_comparable_with<value_type, T2> &&
                 std::equality_comparable_with<error_type, E2>)
    friend constexpr bool operator==(const expected& x,
                                     const expected<T2, E2>& y)
    {
        if (x.has_value() != y.has_value()) {
            return false;
        }
        if (x.has_value()) {
            return x.value() == y.value();
        } else {
            return x.error() == y.value();
        }
    }

    template<class T2>
        requires std::equality_comparable_with<value_type, T2>
    friend constexpr bool operator==(const expected& self, const T2& value)
    {
        if (!self.has_value()) {
            return false;
        }
        return self.value() == value;
    }

    template<class E2>
        requires std::equality_comparable_with<error_type, E2>
    friend constexpr bool operator==(const expected& self,
                                     const unexpected<E2>& error)
    {
        if (self.has_value()) {
            return false;
        }
        return self.error() == error;
    }

  private:
    std::variant<value_type, error_type> data;
};

// partial specialization of expected for void types
template<class T, class E>
    requires std::is_void_v<T>
class expected<T, E>;

}

#endif

#endif // EXPECTED_HPP_INCLUDED
