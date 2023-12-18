// system.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_SYSTEM_HPP
#define INCLUDED_SYSTEM_HPP

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <source_location>
#include <string>
#include <system_error>
#include <span>
#include <vector>
#include <chrono>

#include <unistd.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <poll.h>

#include "debug.hpp"

namespace vb {

namespace fs = std::filesystem;

namespace sys {

enum class call_type {
    ERRNO,
    RETURN_ERRNO,
    SIGNAL
};

template <call_type TYPE, typename... ARGS, typename INVOCABLE, std::size_t IGNORED_SIZE = 0>
requires std::invocable<INVOCABLE, ARGS...>
constexpr auto throw_on_error(std::string_view name, INVOCABLE invocable, std::array<int, IGNORED_SIZE> ignored = {}) {
    return [=](ARGS... args, std::source_location source = std::source_location::current()) {
        auto error = [=](auto err) { 
            auto code = std::error_code(err, std::system_category());
            return std::system_error(code, 
                std::string(name) +
                " call_type::ERRNO = " + std::to_string(errno) +
                " ⇒ " +
                code.message() +
                " at " +
                source.function_name() +
                " → " +
                source.file_name() + ":" + std::to_string(source.line()) + ":" + std::to_string(source.column()));
        };

        auto result = invocable(args...);
        using result_t = decltype(result);
        debug(name, "( ");
        (debug(args, " "), ...);
        debug.log_to(std::cerr, " ) = ", result);

        int result_error = 0;
        if constexpr (TYPE == call_type::ERRNO) {
            if (result_error == -1) {
                result_error = errno;
            }
        } else if constexpr (TYPE == call_type::RETURN_ERRNO) {
            result_error = result;
        } else if constexpr (TYPE == call_type::SIGNAL) {
            if (result == SIG_ERR) {
                result_error = errno;
            }
        }

        if constexpr (IGNORED_SIZE != 0) {
            if (std::find(std::begin(ignored), std::end(ignored), errno) != std::end(ignored)) {
                return result;
            }
        }
        if constexpr (std::integral<result_t>)
        {
            if (result == -1) {
                throw error(errno);
            }
        } else {
            if (errno != 0) {
                throw error(errno);
            }
        }
        return result;
    };
}

template <typename ARG_TYPE>
concept is_arguments_type = std::ranges::viewable_range<ARG_TYPE> &&
requires(ARG_TYPE args)
{
    { std::data(*std::begin(args)) } -> std::same_as<char*>;
};

struct Args {
    using c_holder = std::vector<char *>;
    std::vector<std::string> data_source;
    c_holder c_data;

    template <is_arguments_type ARG_T>
    Args(const ARG_T& args) :
        data_source{ std::begin(args), std::end(args) },
        c_data{}
    {
        update_c();
    }

    void update_c()
    {
        c_data.clear();
        std::ranges::copy(data_source | std::views::all | std::views::transform([](auto& val) { return val.data(); }), std::back_inserter(c_data));
    }

    auto get_c_pointer() const
    {
        return c_data.data();
    }

    void push_front(is_string auto value)
    {
        data_source.insert(std::begin(data_source), value);
        update_c();
    }

    auto data() const
    {
        return c_data.data();
    }
};

auto exec(fs::path exe, is_arguments_type auto arguments, std::source_location source = std::source_location::current())
{
    constexpr auto exec = throw_on_error<call_type::ERRNO, const char*, char* const[]>("execv", ::execv);
    auto executable = exe.string();
    auto args_arr = Args(arguments);
    args_arr.push_front(executable.data()) ;

    debug.log_to(std::cerr, "exec( ", args_arr, ") ");

    return exec(executable.c_str(), args_arr.data(), source);
}

inline auto pipe(std::source_location source = std::source_location::current())
-> std::array<int,2>
{
    std::array<int, 2> result{-1, -1};
    throw_on_error<call_type::ERRNO>("pipe", [&result]() {
        return ::pipe(result.data());
    })(source);
    return result;
}

using status_type = std::optional<int>;

inline auto wait_pid(pid_t pid, int option = 0, std::source_location source = std::source_location::current())
-> status_type
{
    constexpr auto sys_waitpid = throw_on_error<call_type::ERRNO, pid_t, int*, int>("waitpid", ::waitpid, std::array{ ECHILD });
    int status{0};
    int pid_r = sys_waitpid(pid, &status, option, source);
    if (pid_r != pid || !WIFEXITED(status)) {
        return status_type{};
    }
    return WEXITSTATUS(status);
}

inline auto status_pid(pid_t pid, std::source_location source = std::source_location::current())
-> status_type
{
    return wait_pid(pid, WNOHANG, source);
}

struct poll_arg {
    int fd;
    short events;

    operator ::pollfd() const
    {
        return pollfd{
            fd,
            events,
            0};
    }
};

template <std::same_as<poll_arg>... Ts>
auto poll(std::chrono::milliseconds timeout, Ts... fd_s)
{
    std::array<struct pollfd, sizeof...(Ts)> pollfds {fd_s...};
    static auto poll = throw_on_error<call_type::ERRNO, struct pollfd*, nfds_t, int>("poll", ::poll);

    poll(pollfds.data(), pollfds.size(), static_cast<int>(timeout.count()));
    std::array<short, sizeof...(Ts)> result;
    std::ranges::copy(pollfds | std::views::transform(&::pollfd::revents), result.begin());
    return result;
}

constexpr inline auto dup     = throw_on_error<call_type::ERRNO, int>                          ("dup",    ::dup);
constexpr inline auto dup2    = throw_on_error<call_type::ERRNO, int, int>                     ("dup2",   ::dup2);
constexpr inline auto fork    = throw_on_error<call_type::ERRNO>                               ("fork",   ::fork);
constexpr inline auto signal  = throw_on_error<call_type::SIGNAL, int, void(*)(int)>           ("signal", ::signal);
constexpr inline auto read    = throw_on_error<call_type::ERRNO, int, void*, std::size_t>      ("Read",   ::read);
constexpr inline auto write   = throw_on_error<call_type::ERRNO, int, const void*, std::size_t>("write",  ::write);
constexpr inline auto close   = throw_on_error<call_type::ERRNO, int>                          ("close",  ::close);
constexpr inline auto open    = throw_on_error<call_type::ERRNO, const char*, int>             ("open",   ::open);

class spawn {
    posix_spawn_file_actions_t file_actions{};
    posix_spawnattr_t    attributes{};

    static constexpr auto posix_spawn {
        throw_on_error<
            call_type::RETURN_ERRNO,
            ::pid_t*,
            const char*,
            const posix_spawn_file_actions_t*,
            const posix_spawnattr_t*,
            char * const *,
            char * const *
       >("posix_spawn", ::posix_spawn)
    };

    constexpr static auto spawn_file_actions_init{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawn_file_actions_t*>("posix_spawn_file_actions_init", ::posix_spawn_file_actions_init)
    };
    constexpr static auto spawn_file_actions_destroy{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawn_file_actions_t*>("posix_spawn_file_actions_destroy", ::posix_spawn_file_actions_destroy)
    };
    constexpr static auto spawn_file_actions_addchdir{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawn_file_actions_t*, const char *>("posix_spawn_file_actions_addchdir", ::posix_spawn_file_actions_addchdir_np)
    };
    constexpr static auto spawn_file_actions_addclose{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawn_file_actions_t*, int>("posix_spawn_file_actions_addclose", ::posix_spawn_file_actions_addclose)
    };
    constexpr static auto spawn_file_actions_adddup2{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawn_file_actions_t*, int, int>("posix_spawn_file_actions_adddup2", ::posix_spawn_file_actions_adddup2)
    };

    constexpr static auto spawnattr_init{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawnattr_t*>("posix_spawnattr_init", ::posix_spawnattr_init)
    };
    constexpr static auto spawnattr_destroy{
        throw_on_error<call_type::RETURN_ERRNO, posix_spawnattr_t*>("posix_spawnattr_destroy", ::posix_spawnattr_destroy)
    };

public:

    spawn(std::source_location source = std::source_location::current())
    {
        spawn_file_actions_init(&file_actions, source); 
        spawnattr_init(&attributes, source);
    }

    spawn(const spawn&)            = delete;
    spawn(spawn&&)                 = delete;
    spawn& operator=(const spawn&) = delete;
    spawn& operator=(spawn&&)      = delete;

    ~spawn()
    {
        spawn_file_actions_destroy(&file_actions);
        spawnattr_destroy(&attributes);
    }

    void cwd(fs::path dir, std::source_location source = std::source_location::current()) {
        spawn_file_actions_addchdir(&file_actions, dir.native().c_str(), source);
    }

    void setup_dup2(int fromFd, int toFd, std::source_location source = std::source_location::current()) {
        spawn_file_actions_adddup2(&file_actions, fromFd, toFd, source);
    }

    void add_close(int fd, std::source_location source = std::source_location::current()) {
        spawn_file_actions_addclose(&file_actions, fd, source);
    }

    void move_fd(int fromFd, int toFd, std::source_location source = std::source_location::current()) {
        setup_dup2(fromFd, toFd, source);
        add_close(fromFd, source);
    }

    int operator()(fs::path exec, is_arguments_type auto args, std::source_location source = std::source_location::current())
    {
        pid_t pid{-1};
        auto c_args = Args(args);
        posix_spawn(&pid, exec.native().c_str(), &file_actions, &attributes, c_args.data(), nullptr, source); 
        return pid;
    }

    int operator()(fs::path exec, is_arguments_type auto args, is_arguments_type auto env, std::source_location source = std::source_location::current())
    {
        pid_t pid{-1};
        auto c_args = Args(args);
        auto c_env = Args(env);
        auto executable = exec.native();
        posix_spawn(&pid, executable.c_str() , &file_actions, &attributes, c_args.data(), c_env.data(), source); 
        return pid;
    }

};

}}

#endif
