// generator.hpp                                                                        -*-C++-*-
#ifndef INCLUDED_GENERATOR_HPP
#define INCLUDED_GENERATOR_HPP

#include <coroutine>
#include <optional>

namespace vb {

template <typename VALUE_TYPE, typename REFERENCE_TYPE> 
struct generator {
    using value_type = VALUE_TYPE;
    using reference =  REFERENCE_TYPE;

    struct promisse_type;
    using handle_type = std::coroutine_handle<promisse_type>;

    handle_type handle;

    struct promisse_type {
        std::optional<value_type> current;
        std::exception_ptr exception;

        generator get_return_object()
        {
            return handle_type::from_promise(*this);
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always   final_suspend() { return {}; }

        template <std::convertible_to<reference> YIELDED_TYPE>
        std::suspend_always yield_value(YIELDED_TYPE && yielded) 
        {
            current = std::forward<YIELDED_TYPE>(yielded);
            return {};
        }
    };

    const auto& current()
    {
        return handle.promisse().current;
    }

    bool resume() 
    {
        if (!current().has_value()) {
            handle();
            return true;
        }
        return false;
    }

    value_type next()
    {
        resume();
        return std::move(current().value());
    }

    explicit operator bool()
    {
        next();
        return handle.done();
    }

    struct iterator {
        using value_type = value_type;
        using reference = reference;

        generator *self = nullptr;
        reference ref{};

        value_type operator*() const {
            return ref;
        }

        iterator& operator++() {
            ref = self->next();
            return *this;
        }

        bool operator==(const iterator&) const {
            return self != nullptr && *self;
        }
    };

    iterator begin() {
        return iterator{this};
    }
};

}

#endif
