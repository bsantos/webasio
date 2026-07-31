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
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>


namespace webasio::coro {

/// Tag holding the target executor for a @ref post request. @internal
template<class Executor>
struct post_t {
    std::decay_t<Executor> executor;
};

/**
 * @brief Rebinds the coroutine to @p executor, always re-queuing the resume.
 *
 * @details
 * `co_await post(ex)` sets the coroutine's associated executor to @p ex and
 * resumes on it using `boost::asio::post` semantics: the continuation is
 * always queued for later execution, never run inline, guaranteeing the
 * current stack unwinds first. Use this (instead of @ref dispatch) when you
 * need to avoid deep recursion or ensure fair scheduling.
 *
 * @tparam Executor The executor (or strand) type to switch to.
 * @param executor The executor to associate with and resume on.
 * @return An awaitable tag consumed by the coroutine machinery.
 *
 * @see webasio::coro::dispatch
 */
template<class Executor>
inline post_t<Executor> post(Executor&& executor) noexcept
{
    return { std::forward<Executor>(executor) };
}

namespace detail {

/// Awaitable that hops the coroutine onto its executor via `asio::post`. @internal
struct post_awaitable : std::suspend_always {
    boost::asio::any_io_executor const& executor_;

    post_awaitable(boost::asio::any_io_executor const& ex)
        : executor_ { ex }
    {}

    void await_suspend(std::coroutine_handle<> h) const noexcept
    {
        boost::asio::post(executor_, detail::dispatch_handler<> { h });
    }
};

/// Maps `co_await post(ex)` to @ref post_awaitable. @internal
template<class Executor>
struct promise_awaitable<post_t<Executor>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, post_t<Executor> arg)
    {
        frame.set_executor(std::move(arg.executor));
        return post_awaitable { frame.executor };
    }
};

} // namespace detail
} // namespace webasio::coro
