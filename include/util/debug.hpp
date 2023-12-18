// debug.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_DEBUG_HPP
#define INCLUDED_DEBUG_HPP

#include <concepts>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <unistd.h>
#include <type_traits>
#include <ranges>

#include "string.hpp"

namespace vb {

#if defined(NDEBUG)
inline constexpr auto debug_enabled = false;
#else
inline constexpr auto debug_enabled = true;
#endif


template <typename TYPE>
concept is_debugable = requires(const TYPE& value, std::ostream& out) {
    { out << value } -> std::same_as<std::ostream&>;
};

struct no_debug {
    void updatepid() {}

    template <typename... ARGS>
    void operator()(ARGS...) {}

    void flush_to(std::ostream&) {}

    template <typename... ARGS>
    void log_to(std::ostream&, ARGS...) {}
};

struct debugger {
    std::string current;
    int pid = ::getpid();

    void updatepid()
    {
        if (pid == -1)
        {
            pid = ::getpid();
        }
    }

    template <typename FIRST, typename... ARGS>
    void operator()(const FIRST& first, const ARGS&... args)
    {
        updatepid();
        if constexpr (std::ranges::range<FIRST> && !is_string_type<FIRST>) {
            std::stringstream out{};
            std::ranges::copy(first | std::views::filter([](const auto& value) {
                if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(value)>>) {
                    return value != nullptr;
                } else {
                    return true;
                }
            }), std::ostream_iterator<std::ranges::range_value_t<FIRST>>(out, ", "));
            current.append(out.str());
        } else if constexpr ( std::same_as<FIRST, char* const *>) {
            auto p = first;
            while (p != nullptr) {
                (*this)(*p);
                p++;
            }
        } else if constexpr (is_string_type<FIRST>) {
            current.append(first); 
        } else if constexpr (requires { {std::to_string(first)}; }) {
            current.append(std::to_string(first));
        } else if constexpr (std::convertible_to<FIRST, const char*>) {
            if (first == nullptr) {
                current.append("«nullptr»");
            } else {
                current.append("\"");
                current.append(first);
                current.append("\"");
            }
        } else {
            current.append("«?»");
        }
        if constexpr (sizeof...(ARGS) > 0) {
            return (*this)(args...);
        }
    }

    void flush_to(std::ostream& out)
    {
        const auto process = pid == ::getpid() ? "Parent " : "Child  ";
        out << process << "| " << current << "\n";
        current = std::string();
    }

    template <typename... ARGS>
    void log_to(std::ostream& out, const ARGS&... args)
    {
        (*this)(args...);
        flush_to(out);
    }
};

using debug_type = std::conditional_t<debug_enabled, debugger, no_debug>;

inline auto debug = debug_type{};

} // namespace vb

#endif
