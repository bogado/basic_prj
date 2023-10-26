#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "generator.hpp"
#include "system.hpp"
#include "pipe.hpp"

#include <iostream>

#include <array>
#include <concepts>
#include <string>

namespace vb {

namespace fs = std::filesystem;

struct execution {
private:
    using pipe_t = pipe<>;
    pipe<> std_out;
    pid_t pid;

    template <pipe_t execution::* INPUT>
    generator<std::string> lines() {
        static std::atomic<bool> done = false;
        auto& input = this->*INPUT;

        while (!current_status.has_value() || input.has_data())
        {
            if (auto val = input(); val) {
                co_yield val.value();
            }
            if (!input.has_data() && done) {
                break;
            }
            current_status = sys::status_pid(pid);
        }
    }

    template <std::size_t SIZE>
    auto execute(fs::path exe, std::array<std::string, SIZE> args, std::source_location source = std::source_location::current())
    {
        if(pid == 0) {
            std_out.redirect_out();
            std::cerr << "Will exec now : " << exe << "(";
            for (auto a : args) {
                std::cerr << "'" << a << "' ";
            };
            std::cerr << ")\n";
            return sys::exec(exe, args, source);
        }
        return 0;
    }

public:
    template <std::size_t SIZE_T>
    execution(fs::path exe, std::array<std::string, SIZE_T> args, std::source_location source = std::source_location::current()) :
        pid(sys::fork()),
        std_out{}
    {
        execute(exe, std::move(args), source);
    }

    execution(fs::path exe, std::source_location source = std::source_location()) :
        execution(exe, std::array<std::string, 0>{}, source)
    {}
       
    auto stdout_lines()
    {
        return lines<&execution::std_out>();
    }

    auto wait()
    {
        while (!current_status.has_value()) {
            current_status = sys::wait_pid(pid);
        }
        return current_status.value(); // NOLINT(bugprone-unchecked-optional-access) // Actually checked above
    }

    auto status()
    {
        current_status = sys::status_pid(pid);
        return current_status;
    }
};

}

#endif
