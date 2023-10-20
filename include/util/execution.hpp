#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "generator.hpp"
#include "system.hpp"
#include "pipe.hpp"

#include <iostream>
#include <sys/wait.h>

#include <future>
#include <filesystem>
#include <expected>
#include <csignal>
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

        auto old = sys::signal(SIGCHLD, [](int) {
            done = true;
        });

        auto& input = this->*INPUT;

        while(!input.closed()) {
            if (auto val = input(); val) {
                co_yield val.value();
            }
            if (!input.has_data() && done) {
                break;
            }
        }
        sys::signal(SIGCHLD, old);
    }

    auto execute(fs::path exe, std::same_as<std::string> auto... args)
    {
        if(pid == 0) {
            std_out.redirect_out();
            std::cout << "Will exec now : " << exe << "\n";
            return sys::exec(exe, args...);
        }
        return 0;
    }

public:
    execution(fs::path exe, std::same_as<std::string> auto... args) :
        std_out{},
        pid(sys::fork())
    {
        execute(exe, args...);
    }

    auto stdout_lines() {
        return lines<&execution::std_out>();
    }

    auto wait() {
        int stat_loc = sys::waitpid(pid, 0);
        return WEXITSTATUS(stat_loc);
    }

    auto exit_stat() {
        return std::async([&]() { return execution::wait(); });
    }
};

}

#endif
