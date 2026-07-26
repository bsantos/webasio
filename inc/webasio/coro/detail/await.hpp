//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/basic_eager.hpp>
#include <webasio/coro/promise.hpp>


namespace webasio::coro::detail {

struct init_await {
    template<class... Ts>
    static auto get_handle(promise<Ts...>&& coro)
    {
        return std::move(coro.m_coro);
    }

    template<class Handler, class Frame>
    struct awaitable {
        std::decay_t<Handler> handler_;
        std::coroutine_handle<Frame> callee_;

        constexpr bool await_ready() const noexcept { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept
        {
            callee_.promise().on_initial_resume(caller);
            return callee_;
        }

        void await_resume()
        {
            unique_handle<Frame> h { callee_ };
            std::move(handler_)(std::move(h->result));
        }
    };

    template<class Handler, class Frame>
    static basic_eager start_coro(Handler&& handler, unique_handle<Frame> coro)
    {
        co_await awaitable<Handler, Frame> { std::forward<Handler>(handler), coro.release() };
    }

    template<class Handler, class Callable>
    static basic_eager start_coro(Handler&& handler, Callable coro)
    {
        using frame_t = typename std::invoke_result_t<Callable>::promise_type;

        co_await awaitable<Handler, frame_t> { std::forward<Handler>(handler), get_handle(coro()).release() };
    }

    template<class Handler, class Frame>
    void operator()(Handler&& handler, unique_handle<Frame> coro)
    {
        start_coro(std::forward<Handler>(handler), std::move(coro));
    }

    template<class Handler, class Callable>
    void operator()(Handler&& handler, Callable&& callable)
    {
        start_coro(std::forward<Handler>(handler), std::forward<Callable>(callable));
    }
};

} // namespace webasio::coro::detail
