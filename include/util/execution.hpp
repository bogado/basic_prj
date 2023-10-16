#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "generator.hpp"

#include <unistd.h>
#include <sys/wait.h>

#include <array>
#include <atomic>
#include <filesystem>
#include <system_error>
#include <expected>
#include <cerrno>
#include <csignal>
#include <iterator>

namespace vb {
static constexpr auto KB = std::size_t{1024};

namespace fs = std::filesystem;

template <std::size_t BUFFER_SIZE = (4 * KB)>
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

private:
    std::array<int,2> file_descriptors{-1,-1};
    buffer_type buffer;

    auto buffer_load(char* data, std::size_t size) {
        auto read_size = ::read(file_descriptors[0], data, size);

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
            throw std::system_error(std::error_code(errno, std::system_category()));
        }
        return read_size;
    };

    auto buffer_load() {
        return buffer.load([this](char* data, std::size_t size) { return buffer_load(data, size); });
    }

public:
    void redirect_out() 
    {
        ::dup2(file_descriptors[1], 1);
        ::close(file_descriptors[1]);
        ::close(file_descriptors[0]);
    }

    void reader() {
        ::close(file_descriptors[1]);
    }

    void close() {
        for (auto& fd : file_descriptors) {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }
    }

    bool closed() const {
        return file_descriptors[0] == -1 || file_descriptors[1] == -1;
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


inline generator<std::string> run_file(fs::path file, std::same_as<std::string> auto ... args)
{
    pipe input;
    int pid = ::fork();
    if (pid == -1) // 
    {
        throw std::system_error(std::error_code(errno, std::system_category()));
    }
    if (pid == 0) // child
    {
        input.redirect_out();
        ::execl(file.string().c_str(), args.c_str()..., nullptr);
    } else
    {
        static std::atomic<bool> done = false;
        input.reader();

        signal(SIGCHLD, [](int) {
            done = true;
        });

        while(!input.closed()) {
            if (auto val = input(); val) {
                co_yield val.value();
            } else {
                throw std::system_error(val.error());
            }
            if(done) {
                break;
            }
        }
    }

}

}

#endif
