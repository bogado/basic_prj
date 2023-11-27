// pipe.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_PIPE_HPP
#define INCLUDED_PIPE_HPP

#include "system.hpp"
#include "buffer.hpp"
#include "./converters.hpp"

#include <utility>
#include <array>
#include <expected>
#include <ranges>
#include <cstddef>

#include <unistd.h>

namespace vb {

enum class io_direction {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    BOTH = 3
};

io_direction operator bitor(io_direction rhs, io_direction lhs)
{
    return static_cast<io_direction>(std::to_underlying(rhs) bitor std::to_underlying(lhs));
}

io_direction operator bitand(io_direction rhs, io_direction lhs)
{
    return static_cast<io_direction>(std::to_underlying(rhs) bitand std::to_underlying(lhs));
}

template <io_direction DIR>
constexpr auto inline inverse_direction = 
( DIR == io_direction::READ
    ? io_direction::WRITE
    : ( DIR == io_direction::WRITE
        ? io_direction::READ
        : io_direction::BOTH
    )
);

template <std::size_t BUFFER_SIZE = (4 * KB)>
struct pipe {
    using buffer_type = vb::buffer_type<BUFFER_SIZE>;
    using enum io_direction;

    template <io_direction DIR>
    static constexpr auto not_for = DIR == READ? WRITE : DIR == WRITE ? READ : BOTH;

private:
    template <io_direction DIR>
    static constexpr auto idx = DIR == READ ? 0 : 1; // index

    std::array<int,2> file_descriptors{-1,-1};
    buffer_type buffer;

    constexpr io_direction direction() const 
    {
        io_direction result = NONE;
        if (file_descriptors[0] > 0) {
            result = READ;
        }
        if (file_descriptors[1] > 0) {
            result = result | WRITE;
        }
        return result;
    }

    auto buffer_load(char* data, std::size_t size)
        -> long
    {
        if (closed()) {
            return 0;
        }

        long read_size = 0;

        if (!is<READ>()) {
            return read_size;
        }

        read_size = sys::read(file_descriptors[idx<READ>], data, size);

        if (read_size == 0) {
            close<READ>();
        } else if (read_size < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                read_size = 0;
                return read_size;
            }
        }
        return read_size;
    };

    auto buffer_load() {
        return buffer.load([this](char* data, std::size_t size) { return buffer_load(data, size); });
    }

    void redirect_fd(int& to_fd, int updated_fd) 
    {
        if (to_fd == updated_fd) {
            return;
        }

        auto new_fd = sys::dup2(to_fd, updated_fd);

        if (to_fd > 2) { // close original fd if it's not std_in, std_out or std_err.
            std::cerr << "Closing " << to_fd << "\n";
            sys::close(to_fd);
        }

        to_fd = new_fd;
    }

public:
    template <io_direction DIR>
    void redirect(int fd)
    {
        set_direction<DIR>();
        redirect_fd(file_descriptors[idx<DIR>], fd);
    }

    // TODO: Support for writting.
    void redirect_in()
    {
        redirect<READ>(0);
    }

    void redirect_out()
    {
        redirect<WRITE>(1);
    }

    void redirect_err()
    {
        redirect<WRITE>(2);
    }

    template <io_direction DIR>
    void close()
    {
        sys::close(idx<DIR>);
    }

    void close_all()
    {
        for (auto& fd : file_descriptors) {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }
    }

    template <io_direction DIR>
    void set_direction()
    {
        if constexpr (DIR == BOTH) {
            throw std::logic_error("Set direction for BOTH is meanless");
        }

        sys::close(file_descriptors[idx<inverse_direction<DIR>>]);
        file_descriptors[idx<inverse_direction<DIR>>] = -1;
    }

    template <io_direction DIR> 
    bool is() const
    {
        return direction() == BOTH || direction() == DIR;
    }

    bool closed() const {
        return (is<READ>() && file_descriptors[0] == -1) && (is<WRITE>() && file_descriptors[1] == -1);
    }

    bool has_data() const {
        return buffer.has_data();
    }

    template <parse::can_be_outstreamed... DATA_Ts>
    auto operator()(DATA_Ts... data)
    {
        for (auto str : std::array{ parse::to_string(data)... })
        {
            sys::write(file_descriptors[idx<WRITE>], str.data(), str.size());
            sys::write(file_descriptors[idx<WRITE>], "\n", 1);
        }
    }

    std::expected<std::string, std::error_code> operator()()
    {
        std::string result;

        while (result.size() == 0 && result.back() != '\n')
        {
            if (!has_data() && !buffer_load())
            {
                break;
            }

            result += buffer.unload_line();
        }
        return result;
    }

    pipe()
    {
        ::pipe(file_descriptors.data());
    }

    pipe(const pipe&) = delete;
    pipe(pipe&& other) noexcept :
        file_descriptors{other.file_descriptors}
    {
        file_descriptors[0] = -1;
        file_descriptors[1] = -1;
    }

    pipe& operator=(const pipe&) = delete;
    pipe& operator=(pipe&& other) noexcept {
        close();
        std::swap(file_descriptors, other.file_descriptors);
    }

    ~pipe()
    {
        close_all();
    }
};


}

#endif
