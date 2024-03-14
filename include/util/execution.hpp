#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "generator.hpp"
#include "system.hpp"
#include "pipe.hpp"

#include <cinttypes>
#include <array>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <string>

namespace vb {

namespace fs = std::filesystem;

enum class std_io : std::uint8_t {
    IN = 0,
    OUT = 1,
    ERR = 2,
};

enum class io_set : std::uint8_t {
    NONE = 0,
    IN = 1,
    OUT = 2,
    ERR = 4
};

template<typename ANY_IO>
concept std_io_or_set = std::same_as<ANY_IO, std_io> || std::same_as<ANY_IO, io_set>;

constexpr auto to_set(io_set io) { return io; }

constexpr auto to_set(std_io io) {
    switch (io)
    {
    case std_io::IN:
        return io_set::IN;
    case std_io::OUT:
        return io_set::OUT;
    case std_io::ERR:
        return io_set::ERR;
    }
}

auto direction(std_io io) {
    if (io == std_io::IN) {
        return io_direction::READ;
    } else {
        return io_direction::WRITE;
    }
}

auto get_fd(std_io io) {
    return static_cast<int>(io);
}

bool operator&(std_io_or_set auto first, io_set second) {
    return (static_cast<std::uint8_t>(to_set(first)) & static_cast<std::uint8_t>(second)) == static_cast<std::uint8_t>(to_set(first));
}

io_set operator|(std_io_or_set auto first, std_io_or_set auto second) {
    return static_cast<io_set>(static_cast<std::uint8_t>(to_set(first)) | static_cast<std::uint8_t>(to_set(second)));
}

struct execution {
private:
    struct redirection_pipes {
        using value_type = std::optional<pipe>;

        redirection_pipes(io_set redirections) : 
        pipes {
           std_io::IN  & redirections ? value_type{pipe{}}: value_type{},
           std_io::OUT & redirections ? value_type{pipe{}}: value_type{},
           std_io::ERR & redirections ? value_type{pipe{}}: value_type{}
        }
        {}
        
        std::array <value_type, 3> pipes;

        value_type& operator[](std_io dir) noexcept {
            auto index = static_cast<uint8_t>(dir);
            return pipes[index]; // NOLINT
        }

        const value_type& operator[](std_io io) const noexcept {
            auto index = static_cast<uint8_t>(io);
            return pipes[index]; // NOLINT
        }

        auto get_fd(std_io io) const noexcept
        -> int {
            const auto& opt = (*this)[io];
            if (!opt) {
                return -static_cast<uint8_t>(io);
            }
            return opt.value().get_fd(direction(io));
        }

        auto get_reverse_fd(std_io io) const noexcept {
            const auto& opt = (*this)[io];
            if (!opt) {
                return -static_cast<uint8_t>(io);
            }
            return opt.value().get_fd(!direction(io));
        }

        bool set_direction(std_io io) noexcept {
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
    };

    redirection_pipes pipes;
    pid_t pid{-1};
    sys::status_type current_status{};
    sys::spawn spawner{};
    sys::lookup path_lookup;

public:
    execution(sys::lookup path = sys::lookup::PATH, io_set redirections = io_set::NONE) :
        pipes{redirections},
        path_lookup{path}
    {}

    template <std_io IO>
    generator<std::string> lines()
    {
        auto& opt_input = pipes[IO];
        if (opt_input.has_value()) {
            auto& input = opt_input.value();
            while (input.has_data() || !status().has_value())
            {
                if (auto val = input(); val)
                {
                    co_yield val.value();
                }
            }
        }
    }

    template <std::size_t SIZE>
    auto execute(
        fs::path exe,
        const std::array<std::string, SIZE>& args,
        fs::path cwd,
        std::source_location source = std::source_location::current())
    {
        spawner.cwd(cwd);
        for (const auto io: { std_io::IN, std_io::OUT, std_io::ERR }) {
            if (pipes[io]) {
                spawner.add_close (pipes.get_fd(io));
                spawner.setup_dup2(pipes.get_reverse_fd(io), get_fd(io));
                spawner.add_close (pipes.get_reverse_fd(io));
                pipes.set_direction(io);
            }
        }

        auto result = spawner(path_lookup, exe, args);

        return result;
    }

    auto execute(fs::path exe, fs::path cwd)
    {
        execute<0>(exe, {}, cwd);
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

    auto done(std_io io) {
        if (pipes[io].has_value()) {
            pipes[io].value().close_all();
        }
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
