// debug.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_DEBUG_HPP
#define INCLUDED_DEBUG_HPP

#include <concepts>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <unistd.h>

#include "string.hpp"

namespace vb {

template <typename TYPE>
concept is_debugable = requires(const TYPE& value, std::ostream& out) {
    { out << value } -> std::same_as<std::ostream&>;
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
            std::copy(std::begin(first), std::end(first), std::ostream_iterator<std::ranges::range_value_t<FIRST>>(out, ", "));
            current.append(out.str());
        } else if constexpr (is_string_type<FIRST>) {
            current.append(first); 
        } else if constexpr (requires { {std::to_string(first)}; }) {
            current.append(std::to_string(first));
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

inline static auto debug = debugger{};

} // namespace vb

#endif
