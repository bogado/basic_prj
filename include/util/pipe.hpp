// pipe.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_PIPE_HPP
#define INCLUDED_PIPE_HPP

#include "system.hpp"
#include "buffer.hpp"
#include "./converters.hpp"

#include <concepts>
#include <utility>
#include <array>
#include <expected>
#include <cstddef>
#include <iostream>

#include <unistd.h>

namespace vb {

enum class io_direction {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    BOTH = 3
};

constexpr inline auto
operator bitor(io_direction rhs, io_direction lhs)
-> io_direction
{
    return static_cast<io_direction>(std::to_underlying(rhs) bitor std::to_underlying(lhs));
}

constexpr inline auto
operator bitand(io_direction rhs, io_direction lhs)
-> io_direction
{
    return static_cast<io_direction>(std::to_underlying(rhs) bitand std::to_underlying(lhs));
}

constexpr inline auto
operator not(io_direction dir)
-> io_direction
{
    using enum io_direction;
    switch(dir)
    {
    case READ:
        return WRITE;
    case WRITE:
        return READ;
    case BOTH:
        return NONE;
    default:
        return BOTH;
    }
}

template <std::size_t BUFFER_SIZE = (4 * KB)>
struct pipe_base {
    using buffer_type = vb::buffer_type<BUFFER_SIZE>;
    using enum io_direction;

private:
    static constexpr auto index(io_direction dir)
    {
        return dir == READ
            ? 0u
            : 1u;
    }

    std::array<int,2> file_descriptors{-1,-1};
    buffer_type buffer;

    constexpr io_direction direction() const 
    {
        io_direction result = NONE;
        if (file_descriptors[index(READ)] > 0) {
            result = READ;
        }
        if (file_descriptors[index(WRITE)] > 0) {
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

        read_size = sys::read(file_descriptors[index(READ)], data, size);

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

    auto buffer_load()
        -> long 
    {
        return buffer.load([this](char* data, std::size_t size) { return buffer_load(data, size); });
    }

    void redirect_fd(int& to_fd, int updated_fd) 
    {
        if (to_fd == updated_fd) {
            return;
        }

        auto new_fd = sys::dup2(to_fd, updated_fd);

        if (to_fd > 2) { // close original fd if it's not std_in, std_out or std_err.
            sys::close(to_fd);
        }

        to_fd = new_fd;
    }

    bool can_be_read() const 
    {
        using namespace std::literals;
        return (sys::poll(0ms, sys::poll_arg {
            .fd = file_descriptors[index(READ)],
            .events = POLLIN})[0] & POLLIN) != 0;
    }

public:
    using expect_string = std::expected<std::string, std::error_code>;
    using unexpected = std::unexpected<std::error_code>;

    constexpr int get_fd(io_direction dir) const
    {
        return file_descriptors.at(index(dir));
    }

    template <io_direction DIR>
    constexpr int get_fd() const
    {
        return get_fd(index(DIR));
    }
    
    template <io_direction DIR>
    void redirect(int fd)
    {
        redirect_fd(file_descriptors[index(DIR)], fd);
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

    void close(io_direction dir)
    {
        if (dir == io_direction::NONE) {
            return;
        }

        if (dir == io_direction::BOTH) {
            close_all();
        }

        auto& fd = file_descriptors.at(index(dir));

        sys::close(fd);
        fd = -1;
    }

    template <io_direction DIR>
    void close()
    {
        close(DIR);
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

    void set_direction(io_direction dir)
    {
        if (dir == BOTH || dir == NONE) {
            throw std::logic_error("Set direction for NONE or BOTH is meanless");
        }

        close(!dir);
    }

    template <io_direction DIR>
    requires(DIR != io_direction::BOTH && DIR != io_direction::NONE)
    auto set_direction()
    {
        return set_direction(DIR);
    }

    template <io_direction DIR> 
    bool is() const
    {
        return direction() == BOTH || direction() == DIR;
    }

    bool closed() const {
        return (is<READ>()  && file_descriptors[index(READ)] == -1)
            && (is<WRITE>() && file_descriptors[index(WRITE)] == -1);
   }

    bool has_data() const {
        return (buffer.has_data() || can_be_read());
    }

    template <parse::can_be_outstreamed... DATA_Ts>
    auto operator()(DATA_Ts... data)
    {
        for (auto str : std::array{ parse::to_string(data)... })
        {
            sys::write(file_descriptors[index(WRITE)], str.data(), str.size());
            if (str.back() != '\n') {
                sys::write(file_descriptors[index(WRITE)], "\n", 1);
            }
        }
    }

    expect_string operator()()
    {
        auto result = std::string();

        while (result.size() == 0 && result.back() != '\n')
        {
            if (!has_data())
            {
                return std::unexpected{ std::error_code(ENODATA, std::system_category()) };
            }

            if (!buffer.has_data() || can_be_read())
            {
                buffer_load();
            }
            result += buffer.unload_line();
        }

        return expect_string{result};
    }

    pipe_base() :
        file_descriptors(sys::pipe())
    {}

    pipe_base(const pipe_base&) = delete;

    pipe_base(pipe_base&& other) noexcept :
        file_descriptors{other.file_descriptors}
    {
        other.file_descriptors = { -1, -1};
    }

    pipe_base& operator=(const pipe_base&) = delete;
    pipe_base& operator=(pipe_base&& other) noexcept {
        close();
        std::swap(file_descriptors, other.file_descriptors);
    }

    ~pipe_base()
    {
        close_all();
    }

    friend std::ostream& operator<<(std::ostream& out, const pipe_base& self) {
        return out << "Pipe{" << self.file_descriptors[0] << ", " << self.file_descriptors[1] << ", " << self.buffer << "}";
    }
};

using pipe = pipe_base<sys::PAGE_SIZE>;

static_assert(std::same_as<std::ostream&, decltype(std::cout << std::declval<vb::pipe>())>);

}

#endif
