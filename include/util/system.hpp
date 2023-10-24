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

namespace vb {

namespace fs = std::filesystem;

static constexpr auto KB = std::size_t{1024};
namespace sys {

static constexpr auto PAGE_SIZE = 4 * KB;
template <typename... ARGS, typename INVOCABLE>
requires std::invocable<INVOCABLE, ARGS...>
constexpr auto throw_on_error(INVOCABLE invocable) {
    return [=](ARGS... args, std::source_location source = std::source_location::current()) {
        auto error = [=](auto err) { 
            auto code = std::error_code(err, std::system_category());
            return std::system_error(code, 
                code.message() +
                " at " +
                source.function_name() +
                " → " +
                source.file_name() + ":" + std::to_string(source.line()) + ":" + std::to_string(source.column()));
        };

        auto result = invocable(args...);
        using result_t = decltype(result);

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
    constexpr auto exec = throw_on_error<const char*, char* const[]>(::execv);
    auto args_arr = std::array<char *, N_ARGS + 2>{};
    auto executable = exe.string();
    args_arr[0] = executable.data();
    std::ranges::copy(data | std::views::transform([](auto& dt) { return dt.data(); }), std::next(std::begin(args_arr)));
    args_arr.back() = nullptr;
    for (auto a: args_arr)  { std::cerr << (a?a:"«nullptr»") << " «←\n"; }
    return exec(executable.c_str(), args_arr.data(), source);
}

auto pipe(std::source_location source = std::source_location::current())
-> std::array<int,2>
{
    constexpr auto pipe = throw_on_error<>([]() {
        int fds[2];
        ::pipe(fds);
        return std::array { fds[0], fds[1] };
    });
    return pipe(source);
}

auto wait_pid(pid_t pid, int option = 0, std::source_location source = std::source_location::current())
-> std::optional<int>
{
    constexpr auto sys_waitpid = throw_on_error<pid_t, int*, int>(::waitpid);
    int status{0};
    sys_waitpid(pid, &status, option, source);
    if (status == 0) {
        return std::optional<int>{};
    }
    return WEXITSTATUS(status);
}

auto status_pid(pid_t pid, std::source_location source = std::source_location::current())
{
    return wait_pid(pid, WNOHANG, source);
}

constexpr auto dup2    = throw_on_error<int, int>(::dup2);
constexpr auto fork    = throw_on_error<>(::fork);
constexpr auto signal  = throw_on_error<int, void(*)(int)>(::signal);
constexpr auto read    = throw_on_error<int, void*, std::size_t>(::read);
constexpr auto write   = throw_on_error<int, const void*, std::size_t>(::write);
constexpr auto close   = throw_on_error<int>(::close);
constexpr auto open    = throw_on_error<const char*, int>(::open);

}}

#endif
