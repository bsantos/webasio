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

struct post_t {
    boost::asio::any_io_executor executor;
};

struct scoped_post_t {
    boost::asio::strand<boost::asio::any_io_executor> executor;
};

template<class Executor>
inline post_t post(Executor&& executor) noexcept
{
    return { std::forward<Executor>(executor) };
}

template<class Executor>
inline scoped_post_t scoped_post(Executor&& executor) noexcept
{
    return { std::forward<Executor>(executor) };
}

namespace detail {

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

template<>
struct promise_awaitable<post_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, post_t arg)
    {
        frame.set_executor(std::move(arg.executor));
        return post_awaitable { frame.get_executor() };
    }
};

template<>
struct promise_awaitable<scoped_post_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, scoped_post_t arg)
    {
        frame.set_strand_executor(std::move(arg.executor));
        return post_awaitable { frame.get_executor() };
    }
};

} // namespace detail

} // namespace webasio::coro
