#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "./filesystem.hpp"
#include "generator.hpp"
#include "pipe.hpp"
#include "system.hpp"
#include "util/environment.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace vb {

namespace fs = std::filesystem;

enum class std_io : std::uint8_t
{
    IN  = 0,
    OUT = 1,
    ERR = 2,
};

enum class io_set : std::uint8_t
{
    NONE = 0,
    IN   = 1,
    OUT  = 2,
    ERR  = 4
};

template<typename ANY_IO>
concept std_io_or_set = std::same_as<ANY_IO, std_io> || std::same_as<ANY_IO, io_set>;

constexpr inline auto
to_set(io_set io)
{
    return io;
}

constexpr inline auto
to_set(std_io io)
{
    switch (io) {
    case std_io::IN:
        return io_set::IN;
    case std_io::OUT:
        return io_set::OUT;
    case std_io::ERR:
        return io_set::ERR;
    }
    throw std::runtime_error("Unreachable");
}

constexpr inline auto
direction(std_io io)
{
    if (io == std_io::IN) {
        return io_direction::READ;
    } else {
        return io_direction::WRITE;
    }
}

constexpr inline auto
get_fd(std_io io)
{
    return static_cast<int>(io);
}

constexpr inline bool
operator&(std_io_or_set auto first, io_set second)
{
    return (static_cast<std::uint8_t>(to_set(first)) & static_cast<std::uint8_t>(second)) ==
           static_cast<std::uint8_t>(to_set(first));
}

constexpr inline io_set
operator|(std_io_or_set auto first, std_io_or_set auto second)
{
    return static_cast<io_set>(static_cast<std::uint8_t>(to_set(first)) | static_cast<std::uint8_t>(to_set(second)));
}

struct execution
{
private:
    struct redirection_pipes
    {
        using value_type = std::optional<pipe>;

        redirection_pipes(io_set redirections)
            : pipes{ std_io::IN & redirections ? value_type{ pipe{} } : value_type{},
                     std_io::OUT & redirections ? value_type{ pipe{} } : value_type{},
                     std_io::ERR & redirections ? value_type{ pipe{} } : value_type{} }
        {
        }

        std::array<value_type, 3> pipes;

        value_type& operator[](std_io dir) noexcept
        {
            auto index = static_cast<uint8_t>(dir);
            return pipes[index]; // NOLINT
        }

        const value_type& operator[](std_io io) const noexcept
        {
            auto index = static_cast<uint8_t>(io);
            return pipes[index]; // NOLINT
        }

        auto get_fd(std_io io) const noexcept -> int
        {
            const auto& opt = (*this)[io];
            if (!opt) {
                return -static_cast<uint8_t>(io);
            }
            return opt.value().get_fd(direction(io));
        }

        auto get_reverse_fd(std_io io) const noexcept
        {
            const auto& opt = (*this)[io];
            if (!opt) {
                return -static_cast<uint8_t>(io);
            }
            return opt.value().get_fd(!direction(io));
        }

        bool set_direction(std_io io) noexcept
        {
            auto& opt = (*this)[io];
            if (!opt) {
                return false;
            }
            if (direction(io) == io_direction::READ) {
                opt.value().set_direction<io_direction::READ>();
            } else {
                opt.value().set_direction<io_direction::WRITE>();
            }
            return true;
        }

        template<std::invocable<std_io, pipe&> EXECUTABLE_T>
        void for_each_pipe(EXECUTABLE_T&& exec)
        {
            for (const auto io : { std_io::IN, std_io::OUT, std_io::ERR }) {
                if (auto& open_pipe = (*this)[io]; open_pipe) {
                    std::forward<EXECUTABLE_T>(exec)(io, open_pipe.value());
                }
            }
        }
    };

    redirection_pipes pipes;
    pid_t             pid{ -1 };
    sys::status_type  current_status{};
    sys::spawn        spawner{};

    auto make_args(fs::path exec, std::ranges::sized_range auto arguments)
    {
        auto result = std::pair{std::vector<std::string>{}, std::vector<const char *>{}};
        result.first.reserve(std::ranges::size(arguments) + 1);
        result.second.reserve(std::ranges::size(arguments) + 1);
        result.first.push_back(exec.string());
        std::ranges::copy(
            arguments | std::views::transform([]<typename T>(const T& dt) {
                if constexpr (std::same_as<std::string, T>) {
                    return dt;
                } else if constexpr (std::constructible_from<std::string, T>){
                    return std::string{dt};
                }
            }),
            std::back_insert_iterator(result.first));
        std::ranges::copy(
            result.first | std::views::transform([](const auto& arg) { return arg.data(); }),
            std::back_insert_iterator(result.second));
        return result;
    }

public:
    execution(io_set redirections = io_set::NONE)
        : pipes{ redirections }
    {
    }

    template<bool BLOCK>
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

    template<std_io IO>
    generator<std::string> lines()
    {
        auto& opt_input = pipes[IO];
        if (opt_input.has_value()) {
            auto& input = opt_input.value();
            while (!input.closed() && !current_status.has_value()) {
                if (auto val = input(); val) {
                    co_yield val.value();
                } else {
                    exec_wait<false>();
                }
            }
        }
    }

    auto execute(
        fs::path                      exe,
        std::ranges::sized_range auto args,
        env::environment::optional    environment = {},
        fs::path                      cwd         = fs::current_path(),
        std::source_location          source      = std::source_location::current())
    {
        spawner.cwd(cwd);
        pipes.for_each_pipe([&](std_io io, vb::pipe& open_pipe) {
            spawner.add_close(open_pipe.get_fd(!direction(io)), source);
            spawner.setup_dup2(open_pipe.get_fd(direction(io)), get_fd(io), source);
            spawner.add_close(open_pipe.get_fd(direction(io)), source);
        });

        auto all_args = make_args(exe, args);

        auto lookup = exe.is_absolute() ? sys::lookup::NO_LOOKUP : sys::lookup::PATH;

        auto environ = environment.has_value() ? environment.value().getEnv() : std::vector<std::string>{};
        auto result  = spawner(lookup, all_args.second, environ);

        pid = spawner.get_pid();

        pipes.for_each_pipe([&](std_io io, vb::pipe& open_pipe) { open_pipe.set_direction(!direction(io)); });
        return result;
    }

    auto execute(
        is_path_like auto          exe,
        env::environment::optional environment = {},
        fs::path                   cwd         = fs::current_path(),
        std::source_location       source      = std::source_location::current())
    {
        return execute(exe, std::array<std::string, 0>{}, environment, cwd, source);
    }

    auto done(std_io io)
    {
        if (pipes[io].has_value()) {
            pipes[io].value().close_all();
        }
    }

    auto wait() -> int { return exec_wait<true>().value_or(-1); }

    auto status() -> sys::status_type { return exec_wait<false>(); }
};

}

#endif
