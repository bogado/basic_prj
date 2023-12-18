#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "generator.hpp"
#include "system.hpp"
#include "pipe.hpp"

#include <array>
#include <concepts>
#include <filesystem>
#include <string>
#include <atomic>

namespace vb {

namespace fs = std::filesystem;

struct execution {
private:
    using pipe_t = pipe<>;
    pipe<> std_out;
    pid_t pid;
    sys::status_type current_status{};

    template <pipe_t execution::* INPUT>
    generator<std::string> lines() {
        auto& input = this->*INPUT;

        while (input.has_data() || !current_status.has_value())
        {
            if (auto val = input(); val) {
                co_yield val.value();
            }
            current_status = sys::status_pid(pid);
        }
    }

    template <std::size_t SIZE>
    auto execute(fs::path exe, std::array<std::string, SIZE> args, fs::path cwd, std::source_location source = std::source_location::current())
    {
        sys::spawn execution_spawn{source};
        execution_spawn.move_fd(std_out.get_fd<io_direction::WRITE>(), 1);
        execution_spawn.cwd(cwd);
        execution_spawn(exe, args);
    }

public:
    template <std::size_t SIZE_T>
    execution(fs::path exe, std::array<std::string, SIZE_T> args, fs::path cwd = fs::current_path(), std::source_location source = std::source_location::current()) :
        std_out{},
        pid()
    {
        execute(exe, std::move(args), cwd, source);
    }

    execution(fs::path exe, std::source_location source = std::source_location()) :
        execution(exe, std::array<std::string, 0>{}, fs::current_path(), source)
    {}

    auto stdout_lines()
    {
        return lines<&execution::std_out>();
    }

    auto wait()
        -> int
    {
        current_status = sys::wait_pid(pid);
        return current_status.value_or(-1);
    }

    auto status()
        -> sys::status_type
    {
        current_status = sys::status_pid(pid);
        return current_status;
    }
};

}

#endif
