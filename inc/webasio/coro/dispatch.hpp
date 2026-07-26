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
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>


namespace webasio::coro {

struct dispatch_t {
    boost::asio::any_io_executor executor;
};

struct scoped_dispatch_t {
    boost::asio::strand<boost::asio::any_io_executor> executor;
};

template<class Executor>
inline dispatch_t dispatch(Executor&& executor) noexcept
{
    return { std::forward<Executor>(executor) };
}

template<class Executor>
inline scoped_dispatch_t scoped_dispatch(Executor&& executor) noexcept
{
    return { std::forward<Executor>(executor) };
}

namespace detail {

struct dispatch_awaitable : std::suspend_always, detail::resume_context<dispatch_awaitable> {
    boost::asio::any_io_executor const& executor_;

    dispatch_awaitable(boost::asio::any_io_executor const& ex)
        : executor_ { ex }
    {}

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept
    {
        boost::asio::dispatch(executor_, detail::dispatch_handler<dispatch_awaitable> { *this, h });

        if (this->resume_awaiting(h))
            return h;

        return std::noop_coroutine();
    }
};

template<>
struct promise_awaitable<dispatch_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, dispatch_t arg)
    {
        frame.set_executor(std::move(arg.executor));
        return dispatch_awaitable { frame.get_executor() };
    }
};

template<>
struct promise_awaitable<scoped_dispatch_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, scoped_dispatch_t arg)
    {
        frame.set_strand_executor(std::move(arg.executor));
        return dispatch_awaitable { frame.get_executor() };
    }
};

} // namespace detail

} // namespace webasio::coro
