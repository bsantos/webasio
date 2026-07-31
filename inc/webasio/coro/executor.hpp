//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/getter_awaitable.hpp>
#include <webasio/coro/detail/promise_frame.hpp>


namespace webasio::coro {

/// Tag type for the @ref executor awaitable. @internal
struct executor_t { };

/**
 * @brief Awaitable that yields the coroutine's current associated executor.
 *
 * @details `co_await executor` returns a
 * `boost::asio::any_io_executor const&` referring to the executor the
 * coroutine is currently running on. Useful to schedule further work or to
 * construct I/O objects bound to the same context.
 *
 * @see webasio::coro::dispatch
 * @see webasio::coro::post
 */
inline constexpr executor_t executor;

namespace detail {

/// Maps `co_await executor` to a getter awaitable. @internal
template<>
struct promise_awaitable<executor_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, executor_t)
    {
        return detail::getter_awaitable<boost::asio::any_io_executor const&> { frame.get_executor() };
    }
};

} // namespace detail
} // namespace webasio::coro
