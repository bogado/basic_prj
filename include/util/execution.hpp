#ifndef INCLUDED_EXECUTION_HPP
#define INCLUDED_EXECUTION_HPP

#include "string.hpp"
#include "generator.hpp"

#include <unistd.h>

#include <array>
#include <filesystem>
#include <system_error>
#include <expected>
#include <cerrno>
#include "generator.hpp"

namespace vb {
static constexpr auto KB = std::size_t{1024};

namespace fs = std::filesystem;

template <std::size_t BUFFER_SIZE = (4 * KB)>
struct pipe {
private:
    std::array<int,2> file_descriptors{-1,-1};

    struct buffer_type {
        using storage_type = std::array<char, BUFFER_SIZE>;
        using const_iterator = storage_type::const_iterator;
        using iterator = storage_type::iterator;
    private:
        storage_type data{};
        iterator free_start = data.begin();
        iterator consumed = free_start;
    public:
        std::size_t free() const {
            return static_cast<std::size_t>(std::distance(const_iterator{free_start}, data.end()));
        }

        std::size_t loaded() const {
            return static_cast<std::size_t>(std::distance(consumed, free_start));
        }

        bool load(std::invocable<char *, std::size_t> auto reader_fn)
        {
            if (consumed != free_start) {
                return true;
            }
            auto read = reader_fn(&*free_start, free());
            if (read < 0) {
                throw std::system_error(std::error_code(errno, std::system_category()));
            }
            std::advance(free_start, read);
            return read > 0;
        }

        std::string unload_line() {
            auto consume_end = std::find(consumed, free_start, '\n');
            auto result = std::string(consumed, consume_end);
            consumed = std::next(consume_end);
            if (consumed == free_start) {
                free_start = data.begin();
                consumed = free_start;
            }
            return result;
        }
    };

    buffer_type buffer;

public:
    void redirect_out() 
    {
        ::dup2(file_descriptors[1], 1);
    }

    void close() {
        for (auto& fd : file_descriptors) {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }
    }

    explicit operator bool() const {
        return file_descriptors[0] != -1 and file_descriptors[1] != -1;
    }

    std::expected<std::string, std::error_code> operator()()
    {
        std::string result;
        while (result.size() == 0 || result.back() != '\n')
        {
            if (!buffer.load([this](char* data, std::size_t size) {
                return ::read(file_descriptors[0], data, size);
            })) {
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

generator<std::string> run_file(fs::path file, std::same_as<std::string> auto ... args)
{
    pipe read;
    int pid = ::fork();
    if (pid == -1) // 
    {
        throw std::system_error(std::error_code(errno, std::system_category()));
    }
    if (pid == 0) // child
    {
        read.redirect_out();
        ::execl(file.string().c_str(), args.c_str()..., nullptr);
    } else
    {
        while(read) {
            if (auto val = read(); val) {
                co_yield val.value();
            } else {
                throw std::system_error(val.error());
            }
        }
    }

}

}

#endif
