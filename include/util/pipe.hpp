// pipe.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_PIPE_HPP
#define INCLUDED_PIPE_HPP

#include "system.hpp"

#include <array>
#include <expected>
#include <ranges>
#include <cstddef>

#include <unistd.h>

namespace vb {

template <std::size_t BUFFER_SIZE = sys::PAGE_SIZE>
struct buffer_type {
    using storage_type = std::array<char, BUFFER_SIZE>;
    using const_iterator = storage_type::const_iterator;
    using iterator = storage_type::iterator;
private:
    storage_type data{};
    iterator free_start = data.begin();
    iterator consumed = free_start;
public:
    bool has_data() const {
        return consumed != free_start;
    }

    std::size_t free() const {
        return static_cast<std::size_t>(std::distance(const_iterator{free_start}, data.end()));
    }

    std::size_t loaded() const {
        return static_cast<std::size_t>(std::distance(consumed, free_start));
    }

    bool load(std::invocable<char *, std::size_t> auto reader_fn)
    {
        auto read = reader_fn(&*free_start, free());
        std::advance(free_start, read);
        return read >= 0;
    }

    std::string unload_line() {
        while(*consumed == '\0' && consumed != free_start)
        {
            consumed++;
        }

        auto consume_end = std::ranges::find(consumed, free_start, '\n');
        auto result = std::string(consumed, consume_end);

        if (consume_end != free_start && *consume_end == '\n') {
            result.append('\n', 1);
            consume_end++;
        }

        consumed = consume_end;
        if (consumed == free_start) {
            free_start = data.begin();
            consumed = free_start;
        }

        return result;
    }
};

template <std::size_t BUFFER_SIZE = (4 * KB)>
struct pipe {
    using buffer_type = vb::buffer_type<BUFFER_SIZE>;

    enum class direction_type {
        READ = 1,
        WRITE = 2,
        BOTH = 3
    };
    using enum direction_type;

    template <direction_type DIR>
    static constexpr auto not_for = DIR == READ? WRITE : DIR == WRITE ? READ : BOTH;

private:
    template <direction_type DIR>
    static constexpr auto idx = DIR == READ ? 0 : 1; // index


    std::array<int,2> file_descriptors{-1,-1};
    buffer_type buffer;

    direction_type direction = BOTH;

    auto buffer_load(char* data, std::size_t size) {
        auto read_size = sys::read(file_descriptors[0], data, size);

        if (closed()) {
            read_size = 0;
            return read_size;
        }
        if (read_size == 0)
        {
            close();
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
    template <direction_type DIR>
    void redirect(int fd) {
        redirect_fd(file_descriptors[idx<DIR>], fd);
        set_direction<DIR>();
    }

    // TODO: Support for writting.
    void redirect_in() {
        redirect<READ>(0);
    }

    void redirect_out() {
        redirect<WRITE>(1);
    }

    void redirect_err() {
        redirect<WRITE>(2);
    }

    void close() {
        for (auto& fd : file_descriptors) {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }
    }

    template <direction_type DIR>
    void set_direction()
    {
        if constexpr (DIR == BOTH) {
            throw std::logic_error("Set direction for BOTH is meanless");
        }

        direction = DIR;
        sys::close(file_descriptors[idx<not_for<DIR>>]);
        file_descriptors[idx<not_for<DIR>>] = -1;
    }

    template <direction_type DIR> 
    bool is() const
    {
        return direction == BOTH || direction == DIR;
    }

    bool closed() const {
        return (is<READ>() && file_descriptors[0] == -1) && (is<WRITE>() && file_descriptors[1] == -1);
    }

    bool has_data() const {
        return buffer.has_data();
    }

    std::expected<std::string, std::error_code> operator()()
    {
        std::string result;
        while (result.size() == 0 && result.back() != '\n')
        {
            if (closed() || (!has_data() && !buffer_load()))
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
        close();
    }
};


}

#endif
