// system.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_SYSTEM_HPP
#define INCLUDED_SYSTEM_HPP

#include <algorithm>
#include <array>
#include <cerrno>
#include <concepts>
#include <filesystem>
#include <iostream>
#include <optional>
#include <ranges>
#include <source_location>
#include <string>
#include <system_error>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "debug.hpp"

namespace vb {

namespace fs = std::filesystem;

static constexpr auto KB = std::size_t{1024};
namespace sys {

static constexpr auto PAGE_SIZE = 4 * KB;
template <typename... ARGS, typename INVOCABLE, std::size_t IGNORED_SIZE = 0>
requires std::invocable<INVOCABLE, ARGS...>
constexpr auto throw_on_error(std::string_view name, INVOCABLE invocable, std::array<int, IGNORED_SIZE> ignored = {}) {
    return [=](ARGS... args, std::source_location source = std::source_location::current()) {
        auto error = [=](auto err) { 
            auto code = std::error_code(err, std::system_category());
            return std::system_error(code, 
                std::string(name) +
                " ERRNO = " + std::to_string(errno) +
                " ⇒ " +
                code.message() +
                " at " +
                source.function_name() +
                " → " +
                source.file_name() + ":" + std::to_string(source.line()) + ":" + std::to_string(source.column()));
        };

        debug(name);
        auto result = invocable(args...);
        using result_t = decltype(result);
        debug("( "); (debug(args, " "), ...); debug.log_to(std::cerr, " ) = ", result);

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

template <std::size_t N_ARGS>
auto exec(fs::path exe, std::array<std::string, N_ARGS> data, std::source_location source = std::source_location::current())
{
    constexpr auto exec = throw_on_error<const char*, char* const[]>("execv", ::execv);
    auto args_arr = std::array<char *, N_ARGS + 2>{};
    auto executable = exe.string();
    args_arr[0] = executable.data();
    std::ranges::copy(data | std::views::transform([](auto& dt) { return dt.data(); }), std::next(std::begin(args_arr)));
    args_arr.back() = nullptr;

    debug.log_to(std::cerr, "exec( ", data, ") ");

    return exec(executable.c_str(), args_arr.data(), source);
}

auto pipe(std::source_location source = std::source_location::current())
-> std::array<int,2>
{
    constexpr auto pipe = throw_on_error<>("pipe", []() {
        int fds[2];
        ::pipe(fds);
        return std::array { fds[0], fds[1] };
    });
    return pipe(source);
}

using status_type = std::optional<int>;

auto wait_pid(pid_t pid, int option = 0, std::source_location source = std::source_location::current())
-> status_type
{
    constexpr auto sys_waitpid = throw_on_error<pid_t, int*, int>("waitpid", ::waitpid, std::array{ ECHILD });
    int status{0};
    sys_waitpid(pid, &status, option, source);
    if (status == 0) {
        return status_type{};
    }
    return WEXITSTATUS(status);
}

auto status_pid(pid_t pid, std::source_location source = std::source_location::current())
-> status_type
{
    return wait_pid(pid, WNOHANG, source);
}

constexpr auto dup2    = throw_on_error<int, int>("dup2", ::dup2);
constexpr auto fork    = throw_on_error<>("fork", ::fork);
constexpr auto signal  = throw_on_error<int, void(*)(int)>("signal", ::signal);
constexpr auto read    = throw_on_error<int, void*, std::size_t>("Read", ::read);
constexpr auto write   = throw_on_error<int, const void*, std::size_t>("write", ::write);
constexpr auto close   = throw_on_error<int>("close", ::close);
constexpr auto open    = throw_on_error<const char*, int>("open", ::open);

}}

#endif
