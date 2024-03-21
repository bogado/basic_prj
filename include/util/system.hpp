// system.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_SYSTEM_HPP
#define INCLUDED_SYSTEM_HPP

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <optional>
#include <ranges>
#include <source_location>
#include <stdexcept>
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

#ifndef __USE_GNU
extern "C" {
    extern char ** environ;
}
#endif // !__USE_GNU

namespace vb {

namespace fs = std::filesystem;

namespace sys {

enum class call_type {
    ERRNO,
    SPAWN,
    SIGNAL
};

template <call_type TYPE, typename... ARGS, typename INVOCABLE, std::size_t IGNORED_SIZE = 0>
requires std::invocable<INVOCABLE, ARGS...>
constexpr auto throw_on_error(std::string_view name, INVOCABLE invocable, std::array<int, IGNORED_SIZE> ignored = {}) {
    return [=](ARGS... args, std::source_location source = std::source_location::current()) {

        auto result = invocable(args...);
        using result_t = decltype(result);
        debug(name, "( ");
        (debug(args, " "), ...);
        debug.log_to(std::cerr, " ) = ", result);

        auto throw_error = [&](auto err) -> result_t {
            if constexpr (IGNORED_SIZE != 0) { 
                if (std::find(std::begin(ignored), std::end(ignored), errno) != std::end(ignored)) {
                    return result;
                }
            }

            std::string result_msg{};
            if constexpr (requires { std::to_string(result); }) {
                result_msg = " → " + std::to_string(result);
            }

            auto code = std::error_code(err, std::system_category());
            throw std::system_error(code, 
             std::string(name) + result_msg +
             " call_type::ERRNO = " + std::to_string(errno) +
             " ⇒ " +
             code.message() +
             " at " +
             source.function_name() +
             " → " +
             source.file_name() + ":" + std::to_string(source.line()) + ":" + std::to_string(source.column())
             );
            return result;
        };

        if constexpr (TYPE == call_type::ERRNO) {
            if (result == -1) {
                return throw_error(errno);
            }
        } else if constexpr (TYPE == call_type::SPAWN) {
            if (result != 0) {
                if (errno == 0) {
                    return throw_error(result);
                } else {
                    return throw_error(errno);
                }
            }
        } else if constexpr (TYPE == call_type::SIGNAL) {
            if (result == SIG_ERR) {
                return throw_error(errno);
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
    Args(fs::path exe, const ARG_T& args) :
        data_source{ std::begin(args), std::end(args) },
        c_data{}
    {
        data_source.insert(std::begin(data_source), exe.native());
        update_c();
    }

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
        c_data.push_back(nullptr);
    }

    auto get_c_pointer() const noexcept
    {
        return c_data.data();
    }

    void push_front(is_string auto value)
    {
        if constexpr (std::same_as<char *, std::remove_cv_t<decltype(value)>>) {
            if (value == nullptr) {
                return;
            }
        }
        data_source.insert(std::begin(data_source), std::string{value});
        update_c();
    }

    auto data() const noexcept
    {
        return c_data.data();
    }

    auto arg0() const
    {
        if (c_data.empty()) {
            throw(std::invalid_argument("No argv[0]"));
        }
        return c_data[0];
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
    constexpr auto sys_waitpid = throw_on_error<call_type::ERRNO, pid_t, int*, int>("waitpid", ::waitpid, std::array{ EAGAIN });
    int status{-1};
    int pid_r = sys_waitpid(pid, &status, option, source);
    if (pid_r != pid) {
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
constexpr inline auto fsync   = throw_on_error<call_type::ERRNO, int>                          ("fsync",  ::fsync);

enum class lookup {
    PATH,
    NO_LOOKUP
};

class spawn {
    posix_spawn_file_actions_t file_actions{};
    posix_spawnattr_t    attributes{};
    pid_t pid{-1};

    template <lookup LOOKUP>
    static constexpr auto posix_spawn {
        throw_on_error<
            call_type::SPAWN,
            ::pid_t*,
            const char*,
            const posix_spawn_file_actions_t*,
            const posix_spawnattr_t*,
            char * const *,
            char * const *
       >(std::string_view("spawnp").substr(0, LOOKUP == lookup::NO_LOOKUP ? 5 : 6), LOOKUP == lookup::NO_LOOKUP ? ::posix_spawn : ::posix_spawnp)
    };

    static constexpr auto function(const lookup path_lookup) {
        if (path_lookup == lookup::NO_LOOKUP) {
            return posix_spawn<lookup::NO_LOOKUP>;
        } else {
            return posix_spawn<lookup::PATH>;
        }
    }

    constexpr static auto spawn_file_actions_init{
        throw_on_error<call_type::SPAWN, posix_spawn_file_actions_t*>("posix_spawn_file_actions_init", ::posix_spawn_file_actions_init)
    };

    constexpr static auto spawn_file_actions_destroy{
        throw_on_error<call_type::SPAWN, posix_spawn_file_actions_t*>("posix_spawn_file_actions_destroy", ::posix_spawn_file_actions_destroy)
    };

    constexpr static auto spawn_file_actions_addchdir{
        throw_on_error<call_type::SPAWN, posix_spawn_file_actions_t*, const char *>("posix_spawn_file_actions_addchdir", ::posix_spawn_file_actions_addchdir_np)
    };

    constexpr static auto spawn_file_actions_addclose{
        throw_on_error<call_type::SPAWN, posix_spawn_file_actions_t*, int>("posix_spawn_file_actions_addclose", ::posix_spawn_file_actions_addclose)
    };

    constexpr static auto spawn_file_actions_adddup2{
        throw_on_error<call_type::SPAWN, posix_spawn_file_actions_t*, int, int>("posix_spawn_file_actions_adddup2", ::posix_spawn_file_actions_adddup2)
    };

    constexpr static auto spawnattr_init{
        throw_on_error<call_type::SPAWN, posix_spawnattr_t*>("posix_spawnattr_init", ::posix_spawnattr_init)
    };

    constexpr static auto spawnattr_destroy{
        throw_on_error<call_type::SPAWN, posix_spawnattr_t*>("posix_spawnattr_destroy", ::posix_spawnattr_destroy)
    };

    auto do_spawn(lookup path_lookup, char *cmd, char * const * args, char * const * env, std::source_location source = std::source_location::current())
    {
        if (env == nullptr) {

            env = ::environ;
        }
        return function(path_lookup)(&pid, cmd, &file_actions, &attributes, args, env, source); 
    }

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

    int operator()(lookup path_lookup, is_arguments_type auto args, std::source_location source = std::source_location::current())
    {
        auto c_args = Args(args);
        return do_spawn(path_lookup, c_args.arg0(), c_args.data(), nullptr, source); 
    }

    int operator()(lookup path_lookup, is_arguments_type auto args, is_arguments_type auto env, std::source_location source = std::source_location::current())
    {
        auto c_args = Args(args);
        auto c_env = Args(env);
        return do_spawn(path_lookup, c_args.arg0(), c_args.data(), c_env.data(), source);
    }

    int operator()(lookup path_lookup, fs::path exec, is_arguments_type auto args, std::source_location source = std::source_location::current())
    {
        auto c_args = Args(exec, args);
        return do_spawn(path_lookup, c_args.arg0(), c_args.data(), nullptr, source);
    }

    int operator()(lookup path_lookup, fs::path exec, is_arguments_type auto args, is_arguments_type auto env, std::source_location source = std::source_location::current())
    {
        auto c_args = Args(exec, args);
        auto c_env = Args(env);
        auto executable = exec.native();
        return do_spawn(path_lookup, c_args.arg0(), c_args.data(), c_env.data(), source);
    }

    auto get_pid() const
    {
        return pid;
    }
};

}}

#endif
