//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/assert.hpp>

#include <exception>
#include <memory>


namespace webasio::coro {

/**
 * @brief Move-only container holding either produced values or an exception.
 *
 * @details
 * `outcome` is the result carrier delivered to callback-style completion
 * handlers (see @ref webasio::coro::await). It stores exactly one of:
 * nothing yet, a value (or tuple of values), or a captured
 * `std::exception_ptr`. Calling `get()` yields the value, re-throwing the
 * stored exception if one is present.
 *
 * The primary template forwards multi-value packs to the tuple
 * specialization; dedicated specializations handle the `void`, empty and
 * single-value cases.
 *
 * @tparam Args The value types carried on success.
 */
template<class... Args>
class outcome;

/**
 * @brief `outcome` specialization for value-less coroutines.
 * @details Carries only success/failure; `get()` re-throws on failure.
 */
template<>
class outcome<void> {
public:
    using value_type = void;

    outcome() noexcept
    {}

    outcome(outcome&& other) noexcept
        : exception_ { std::exchange(other.exception_, nullptr) }
    {}

    outcome& operator=(outcome&& other) noexcept
    {
        if (this != &other)
            exception_ = std::exchange(other.exception_, nullptr);
        return *this;
    }

    void set_value(void)
    {
        assert_trap(!exception_, "outcome already set");
    }

    /// Stores an exception by capturing @p ex into an `exception_ptr`.
    template<class E>
    void set_exception(E&& ex) noexcept
    {
        set_exception(std::make_exception_ptr(std::forward<E>(ex)));
    }

    /// Stores an already-captured exception. @p ex must not be null.
    void set_exception(std::exception_ptr ex) noexcept
    {
        assert_trap(ex != nullptr, "exception cannot be null");
        assert_trap(!exception_, "outcome already set");
        exception_ = ex;
    }

    /// Re-throws the stored exception, if any; otherwise returns normally.
    void get()
    {
        if (exception_)
            std::rethrow_exception(exception_);
    }

private:
    outcome(outcome const&) = delete;
    outcome& operator=(outcome const&) = delete;

    std::exception_ptr exception_;
};

/// `outcome` with an empty pack behaves like @ref outcome<void>.
template<>
class outcome<> : public outcome<void> {};

/**
 * @brief `outcome` specialization holding a single value of type @p T.
 * @details Uses a discriminated union to store either the value or an
 * exception without default-constructing @p T.
 * @tparam T The carried value type (may be a reference).
 */
template<class T>
class outcome<T> {
public:
    using value_type = T;

    constexpr outcome() noexcept
    {}

    outcome(outcome&& other) noexcept(noexcept(storage_t(std::move(other.storage_))))
        : storage_ { std::move(other.storage_) }
    {}

    outcome& operator=(outcome&& other) noexcept(noexcept(std::declval<storage_t>().move(std::move(other.storage_))))
    {
        if (this != &other) {
            storage_.destroy(storage_.which());
            storage_.move(other.storage_);
        }
        return *this;
    }

    /// Constructs the stored value in place from @p args.
    template<class... Args>
    void set_value(Args&&... args) noexcept(noexcept(std::declval<storage_t>().emplace_value(std::forward<Args>(args)...)))
    {
        assert_trap(storage_.which() == what::nothing, "outcome already set");
        storage_.emplace_value(std::forward<Args>(args)...);
    }

    /// Stores an exception by capturing @p ex into an `exception_ptr`.
    template<class E>
    void set_exception(E&& ex) noexcept(noexcept(std::make_exception_ptr(std::forward<E>(ex))))
    {
        set_exception(std::make_exception_ptr(std::forward<E>(ex)));
    }

    /// Stores an already-captured exception. @p ex must not be null.
    void set_exception(std::exception_ptr ex) noexcept
    {
        assert_trap(ex != nullptr, "exception cannot be null");
        assert_trap(storage_.which() == what::nothing, "outcome already set");
        storage_.emplace_exception(std::move(ex));
    }

    /**
     * @brief Extracts the stored value, moving it out.
     * @return The stored value.
     * @throws The stored exception if the outcome holds an error.
     */
    value_type get()
    {
        switch (storage_.which()) {
        case what::value:
            return std::move(storage_.value_);
        case what::exception:
            std::rethrow_exception(storage_.exception_);
        default:
            bug_abort("outcome was not set");
        }
    }

private:
    outcome(outcome const&) = delete;
    outcome& operator=(outcome const&) = delete;

    enum class what {
        nothing,
        value,
        exception,
    };

    union storage_t {
        storage_t() noexcept { what_.v = what::nothing; }
        storage_t(storage_t&& other) noexcept(noexcept(T(std::move(other.value_)))) { move(other); }
        ~storage_t() { destroy(); }

        what which() const noexcept { return what_.v; }

        void move(storage_t& other) noexcept(noexcept(T(std::move(other.value_))))
        {
            switch (other.which()) {
            case what::nothing:
                break;
            case what::value:
                emplace_value(std::move(other.value_));
                break;
            case what::exception:
                emplace_exception(std::move(other.exception_));
                break;
            }
        }

        template<class... Args>
        void emplace_value(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...)))
        {
            std::construct_at(&value_, std::forward<Args>(args)...);
            what_.v = what::value;
        }

        void emplace_exception(std::exception_ptr ex) noexcept
        {
            std::construct_at(&exception_, std::move(ex));
            what_.v = what::exception;
        }

        void destroy() noexcept
        {
            switch (which()) {
            case what::nothing:
                return;
            case what::value:
                std::destroy_at(&value_);
                break;
            case what::exception:
                std::destroy_at(&exception_);
                break;
            }
            what_.v = what::nothing;
        }

        using storage_value = std::conditional_t<std::is_reference_v<T>, std::reference_wrapper<std::remove_reference_t<T>>, T>;

        struct what_storage {
            char padding[sizeof(storage_value) > sizeof(std::exception_ptr) ? sizeof(storage_value) : sizeof(std::exception_ptr)];
            what v;
        };

        what_storage       what_;
        storage_value      value_;
        std::exception_ptr exception_;
    };

    storage_t storage_;
};

/// Multi-value `outcome` storing the values as a `std::tuple`.
template<class... Args>
class outcome : public outcome<std::tuple<Args...>> {};

} // namespace webasio::coro
