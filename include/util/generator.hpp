// generator.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_GENERATOR_HPP
#define INCLUDED_GENERATOR_HPP

#include <coroutine>
#include <iterator>
#include <optional>
#include <exception>

namespace vb {

template <typename VALUE_TYPE, typename REFERENCE_TYPE = const VALUE_TYPE&>
struct generator {
    using value_type = VALUE_TYPE;
    using reference =  REFERENCE_TYPE;

    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    handle_type handle;

    struct promise_type {
        std::optional<value_type> current;
        std::exception_ptr exception;
        bool done = false;

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

        void unhandled_exception() {
            exception = std::current_exception();
        }
    };

    auto& current()
    {
        return handle.promise().current;
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
        if (!current().has_value()) {
            resume();
        }
        auto& opt = current();
        if (!opt.has_value()) {
            return {};
        }
        value_type val = opt.value();
        opt.reset();
        return val;
    }

    explicit operator bool()
    {
        resume();
        return handle.done();
    }

    struct iterator {
        using value_type = generator::value_type;
        using reference = generator::reference;

        mutable generator *self = nullptr;

        iterator(generator* s) :
            self{s}
        {}

        reference operator*() const {
            if (!self->current().has_value()) {
                self->next();
            }
            return self->current().value();
        }

        iterator& operator++() {
            self->next();
            return *this;
        }

        bool operator==(const std::default_sentinel_t&) const {
            return self == nullptr || *self;
        }
    };

    iterator begin() {
        return iterator{this};
    }

    auto end() {
        return std::default_sentinel;
    }
};

}

#endif
