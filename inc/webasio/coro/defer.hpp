//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/dispatch_handler.hpp>
#include <webasio/coro/detail/promise_frame.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/defer.hpp>
#include <boost/asio/strand.hpp>


namespace webasio::coro {

/// Tag holding the target executor for a @ref defer request. @internal
template<class Executor>
struct defer_t {
    std::decay_t<Executor> executor;
};

/**
 * @brief Rebinds the coroutine to @p executor, always re-queuing the resume as
 * a continuation.
 *
 * @details
 * `co_await defer(ex)` sets the coroutine's associated executor to @p ex and
 * resumes on it using `boost::asio::defer` semantics: like @ref post the
 * continuation is always queued and never run inline, but `defer` additionally
 * signals (via `execution::relationship.continuation`) that the resume is a
 * continuation of the current operation. That hint lets the executor keep the
 * work on the current thread and skip the extra synchronisation @ref post may
 * perform. Prefer `defer` over @ref post when the resume logically continues
 * the caller, and over @ref dispatch when inline resumption must be avoided.
 *
 * @tparam Executor The executor (or strand) type to switch to.
 * @param executor The executor to associate with and resume on.
 * @return An awaitable tag consumed by the coroutine machinery.
 *
 * @see webasio::coro::post
 * @see webasio::coro::dispatch
 */
template<class Executor>
inline defer_t<Executor> defer(Executor&& executor) noexcept
{
    return { std::forward<Executor>(executor) };
}

namespace detail {

/// Awaitable that hops the coroutine onto its executor via `asio::defer`. @internal
struct defer_awaitable : std::suspend_always {
    boost::asio::any_io_executor const& executor_;

    defer_awaitable(boost::asio::any_io_executor const& ex)
        : executor_ { ex }
    {}

    void await_suspend(std::coroutine_handle<> h) const noexcept
    {
        boost::asio::defer(executor_, detail::dispatch_handler<> { h });
    }
};

/// Maps `co_await defer(ex)` to @ref defer_awaitable. @internal
template<class Executor>
struct promise_awaitable<defer_t<Executor>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, defer_t<Executor> arg)
    {
        frame.set_executor(std::move(arg.executor));
        return defer_awaitable { frame.executor };
    }
};

} // namespace detail
} // namespace webasio::coro
