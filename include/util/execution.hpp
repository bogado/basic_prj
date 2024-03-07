#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "generator.hpp"
#include "system.hpp"
#include "pipe.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace vb {

namespace fs = std::filesystem;

struct execution {
private:
    static constexpr auto std_in  = std::uint8_t{0};
    static constexpr auto std_out = std::uint8_t{1};
    static constexpr auto std_err = std::uint8_t{2};

    static constexpr auto directions = std::array{
        io_direction::WRITE,
        io_direction::READ,
        io_direction::READ
    };

    std::array <pipe, 3> pipes;
    pid_t pid;
    sys::status_type current_status{};

    template <std::size_t INPUT>
    requires (INPUT >= 0 && INPUT < 3)
    generator<std::string> lines()
    {
        auto& input = pipes[INPUT];

        while (input.has_data() || !status().has_value())
        {
            if (auto val = input(); val) {
                co_yield val.value();
            }
        }
    }

    template <std::size_t SIZE>
    auto execute(fs::path exe,const std::array<std::string, SIZE>& args, fs::path cwd, std::source_location source = std::source_location::current())
    {
        sys::spawn execution_spawn{source};
        execution_spawn.cwd(cwd);
        for (const auto fd: { std_in, std_out, std_err }) {
            execution_spawn.setup_dup2(pipes.at(fd).get_fd(!directions.at(fd)), fd);
            execution_spawn.add_close (pipes.at(fd).get_fd(directions.at(fd)));
            execution_spawn.add_close (pipes.at(fd).get_fd(!directions.at(fd)));
        }

        auto result = execution_spawn(exe, args);

        for (const auto fd: { std_in, std_out, std_err }) {
            pipes.at(fd).set_direction(directions.at(fd));
        }
        return result;
    }

    template <bool BLOCK>
    auto exec_wait()
    {
        auto wait_func = [&]() {
            if constexpr (BLOCK) {
                return sys::wait_pid(pid);
            } else {
                return sys::status_pid(pid);
            }
        };

        if (current_status.has_value()) {
            return current_status;
        }
        current_status = wait_func();
        return current_status;
    }

public:
    template <std::size_t SIZE_T>
    execution(fs::path exe, std::array<std::string, SIZE_T> args, fs::path cwd = fs::current_path(), std::source_location source = std::source_location::current()) :
        pipes{pipe{}, pipe{}, pipe{}},
        pid(execute(exe, std::move(args), cwd, source))
    {}

    execution(fs::path exe, std::source_location source = std::source_location()) :
        execution(exe, std::array<std::string, 0>{}, fs::current_path(), source)
    {}

    auto stderr_lines()
    {
        return lines<std_err>();
    }

    auto stdout_lines()
    {
        return lines<std_out>();
    }

    auto send_line(const is_string_type auto& str)
    {
        auto& in = pipes[std_in];
        in(str);
        in.flush();
    }

    auto wait()
        -> int
    {
        return exec_wait<true>().value_or(-1);
    }

    auto status()
        -> sys::status_type
    {
        return exec_wait<false>();
    }
};

}

#endif
