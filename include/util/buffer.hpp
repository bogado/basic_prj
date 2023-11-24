// buffer.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_BUFFER_HPP
#define INCLUDED_BUFFER_HPP

#include <concepts>
#include <string>
#include <iterator>
#include <ranges>

namespace vb {

constexpr inline auto KB = std::size_t{1024};
constexpr inline auto MB = KB*KB;
constexpr inline auto GB = MB*KB;

namespace sys {
static constexpr auto PAGE_SIZE = 4 * KB;
}

template <typename SYS_READ_FN, typename... EXTRA_ARGS>
concept system_read_fn = std::invocable<SYS_READ_FN, EXTRA_ARGS..., char *, std::size_t> 
    && std::convertible_to<std::invoke_result_t<SYS_READ_FN, EXTRA_ARGS..., char *, std::size_t>, std::size_t>;

template <std::size_t BUFFER_SIZE = sys::PAGE_SIZE>
struct buffer_type {
    using storage_type = std::array<char, BUFFER_SIZE>;
    using const_iterator = storage_type::const_iterator;
    using iterator = storage_type::iterator;
private:
    storage_type data{};
    iterator used_begin = data.begin();
    iterator used_end   = data.begin();

    inline constexpr bool valid_equal(char* what, char value) {
        return what != used_end && *what == value;
    }
public:
    bool has_data() const {
        return used_begin != used_end;
    }

    std::size_t free() const {
        return static_cast<std::size_t>(std::distance(const_iterator{used_end}, data.end()));
    }

    std::size_t loaded() const {
        return static_cast<std::size_t>(std::distance(used_begin, used_end));
    }

    template <typename... ARGS, std::invocable<ARGS..., char*, std::size_t> FUNC_T, std::ptrdiff_t FAILURE = -1>
    auto load(FUNC_T reader_fn, ARGS... args)
    -> ptrdiff_t
    {
        auto read = reader_fn(args..., &*used_end, free());
        if (read == FAILURE) {
            return FAILURE;
        }
        std::advance(used_end, read);
        return read;
    }

    std::string unload_line()
    {
        if (used_begin == used_end) {
            return {};
        }

        auto consume_end = std::ranges::find(used_begin, used_end, '\n');
        auto result = std::string(used_begin, consume_end);

        if (valid_equal(consume_end, '\n')) {
            result.append(1, '\n');
            consume_end++;
            if (valid_equal(consume_end, '\0')) {
                consume_end++;
            }
        }

        if (consume_end == used_end) {
            used_begin = used_end = data.begin();
        } else {
            used_begin = consume_end;
        }

        return result;
    }
};

}

#endif
