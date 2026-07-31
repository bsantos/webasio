//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/promise_frame.hpp>

namespace webasio::coro {

/**
 * @brief Lazy, awaitable coroutine return type carrying values @p Ts.
 *
 * @details
 * A `promise` is the handle returned by a coroutine that uses it as its
 * return type. It is lazy: the coroutine body does not start until the
 * promise is awaited (via `co_await`) or driven through `await()`. When
 * awaited, the values produced by `co_return` are delivered to the caller,
 * and any exception escaping the coroutine body is re-thrown at the
 * `co_await` expression.
 *
 * The type is move-only through its underlying coroutine handle and is
 * marked `[[nodiscard]]` because dropping it would discard the coroutine
 * without ever running it.
 *
 * @tparam Ts The value types produced by the coroutine. An empty pack (or
 * `void`) denotes a coroutine that yields no value.
 *
 * @note Prefer the `webasio::co_promise` alias in user code.
 * @see webasio::coro::await
 * @see webasio::coro::outcome
 */
template<class... Ts>
class [[nodiscard]] promise {
public:
    /// The coroutine promise type driving this return object.
    using promise_type = detail::promise_frame<Ts...>;

    /**
     * @brief Releases ownership of the coroutine and returns its awaitable.
     *
     * @details Consumes the promise, detaching the underlying coroutine
     * handle so it can be awaited directly. Only valid on an rvalue.
     *
     * @return An awaitable that resumes the coroutine and yields its result.
     */
    auto get_awaitable() && noexcept
    {
        return std::exchange(m_coro, nullptr)->get_awaitable();
    }

private:
    template<class...>
    friend struct detail::promise_frame;
    friend struct detail::init_await;

    promise() = delete;
    promise(promise&&) = delete;
    promise(promise const&) = delete;
    promise& operator=(promise&&) = delete;
    promise& operator=(promise const&) = delete;

    promise(promise_type* coro)
        : m_coro { coro }
    {}

    unique_handle<promise_type> m_coro;
};


/**
 * @brief Trait detecting whether @p T is a `promise` specialization.
 * @tparam T The type to inspect.
 */
template<class T>
struct is_promise : std::false_type {};

template<class... Ts>
struct is_promise<promise<Ts...>> : std::true_type {};

/// Convenience variable template for @ref is_promise.
template<class T>
inline constexpr bool is_promise_v = is_promise<T>::value;


/**
 * @brief Maps a `promise<Ts...>` to its corresponding `outcome<Ts...>`.
 *
 * @details The outcome type is the value/exception container delivered to
 * callback-style completion handlers used by `await()`.
 *
 * @tparam T A `promise` specialization.
 */
template<class T>
struct promise_outcome;

template<class... Ts>
struct promise_outcome<promise<Ts...>> {
    using type = outcome<Ts...>;
};

/// Alias for @ref promise_outcome::type.
template<class T>
using promise_outcome_t = typename promise_outcome<T>::type;

} // namespace webasio::coro

namespace webasio {

/// Public alias for @ref webasio::coro::promise.
template<class... Ts>
using co_promise = coro::promise<Ts...>;

} // namespace webasio
