#ifndef INCLUDED_GENERATOR_HPP
#define INCLUDED_GENERATOR_HPP

#include <coroutine>
#include <iterator>
#include <optional>
#include <exception>
#include <cstdint>

namespace vb {

template <typename VALUE_TYPE, typename REFERENCE_TYPE = const VALUE_TYPE&>
struct generator {
    using value_type = VALUE_TYPE;
    using reference =  REFERENCE_TYPE;

    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    handle_type handle;
    value_type current;

    generator(handle_type handle_) :
        handle{handle_},
        current{}
    {}

    struct promise_type {
        std::exception_ptr exception{};
        bool done = false;
        std::optional<value_type> current{};

        generator get_return_object() noexcept
        {
            return generator{handle_type::from_promise(*this)};
        }

        std::suspend_always  initial_suspend() noexcept { return {}; }
        std::suspend_always    final_suspend() noexcept { done = true; return {}; }

        template <std::convertible_to<reference> YIELDED_TYPE>
        std::suspend_always yield_value(YIELDED_TYPE && yielded) noexcept
        {
            current = std::forward<YIELDED_TYPE>(yielded);
            return {};
        }

        void unhandled_exception() 
        {
            exception = std::current_exception();
        }
    };

private:
 
    auto promise()
    {
        return handle.promise();
    }

    bool resume()
    {
        if (handle.done()) {
            return false;
        }

        handle.resume();

        if (auto thrown = handle.promise().exception; thrown) {
            std::rethrow_exception(thrown);
        }
        return true;
    }

    value_type next()
    {
        if (!resume() || !promise().current.has_value()) {
            return {};
        }
        return promise().current.value();
    }
public:

    explicit operator bool()
    {
        return !handle.done();
    }

    struct iterator {
        using value_type = generator::value_type;
        using difference_type = std::ptrdiff_t;
        using size_type = std::size_t;

        mutable generator *self = nullptr;

        iterator(generator* s) :
            self{s}
        {}

        reference operator*() const {
            return self->current;
        }

        iterator operator++(int) {
            auto other = *this;
            self->current = self->next();
            return other;
        }

        iterator& operator++() {
            self->current = self->next();
            return *this;
        }

        bool operator==(const std::default_sentinel_t&) const {
            return self == nullptr || !(*self);
        }
    };

    iterator begin() {
        auto result = iterator{this};
        ++result;
        return result;
    }

    auto end() {
        return std::default_sentinel;
    }
};

}

#endif
