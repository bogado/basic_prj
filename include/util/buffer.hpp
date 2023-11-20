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
    iterator usage_end = data.begin();
    iterator usage_begin = usage_end;
public:
    bool has_data() const {
        return usage_begin != usage_end;
    }

    std::size_t free() const {
        return static_cast<std::size_t>(std::distance(const_iterator{usage_end}, data.end()));
    }

    std::size_t loaded() const {
        return static_cast<std::size_t>(std::distance(usage_begin, usage_end));
    }

    template <typename... ARGS>
    auto load(auto reader_fn, ARGS... args)
    {
        auto read = reader_fn(args..., &*usage_end, free());
        std::advance(usage_end, read);
        return read;
    }

    std::string unload_line() {
        while(*usage_begin == '\0' && usage_begin != usage_end)
        {
            usage_begin++;
        }

        auto consume_end = std::ranges::find(usage_begin, usage_end, '\n');
        auto result = std::string(usage_begin, consume_end);

        if (consume_end != usage_end && *consume_end == '\n') {
            result.append('\n', 1);
            consume_end++;
        }

        usage_begin = consume_end;
        if (usage_begin == usage_end) {
            usage_end = data.begin();
            usage_begin = usage_end;
        }

        return result;
    }
};



}

#endif
