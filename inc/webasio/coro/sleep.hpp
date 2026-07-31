//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/promise_frame.hpp>
#include <webasio/coro/detail/timer_awaitable.hpp>


namespace webasio::coro {

/// Tag carrying a sleep duration or expiry time point. @internal
template<class... Options>
struct sleep_t;

template<class Rep, class Period>
struct sleep_t<std::chrono::duration<Rep, Period>> {
    std::chrono::duration<Rep, Period> duration;
};

template<class Clock, class Rep, class Period>
struct sleep_t<std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>> {
    std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>> expiry_time;
};

/**
 * @brief Suspends the coroutine for a relative @p duration.
 *
 * @details `co_await sleep_for(d)` waits `d` on a steady timer bound to the
 * coroutine's executor, then resumes. The wait is cancellable: if the
 * coroutine's cancellation state is triggered the timer is cancelled and the
 * coroutine resumes early (observe via `co_await cancelled`).
 *
 * @tparam Rep,Period `std::chrono::duration` template parameters.
 * @param duration How long to wait.
 * @return An awaitable tag consumed by the coroutine machinery.
 *
 * @see webasio::coro::sleep_until
 */
template<class Rep, class Period>
constexpr auto sleep_for(std::chrono::duration<Rep, Period> duration)
{
    return sleep_t<std::chrono::duration<Rep, Period>> { duration };
}

/**
 * @brief Suspends the coroutine until an absolute @p expiry_time.
 *
 * @details `co_await sleep_until(tp)` waits until the clock reaches @p tp,
 * using a steady timer for `steady_clock` time points and a generic waitable
 * timer otherwise. The wait is cancellable like @ref sleep_for.
 *
 * @tparam Clock,Rep,Period `std::chrono::time_point` template parameters.
 * @param expiry_time The time point to wake up at.
 * @return An awaitable tag consumed by the coroutine machinery.
 *
 * @see webasio::coro::sleep_for
 */
template<class Clock, class Rep, class Period>
constexpr auto sleep_until(std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>> expiry_time)
{
    return sleep_t<std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>> { expiry_time };
}

namespace detail {

template<class Rep, class Period>
struct promise_awaitable<sleep_t<std::chrono::duration<Rep, Period>>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, sleep_t<std::chrono::duration<Rep, Period>> arg)
    {
        if (!frame.steady_timer)
            frame.steady_timer = std::make_unique<boost::asio::steady_timer>(frame.get_executor());

        return detail::basic_timer_awaitable<promise_frame<Ts...>, boost::asio::steady_timer&> {
            frame, *frame.steady_timer, arg.duration
        };
    }
};

template<class Rep, class Period>
struct promise_awaitable<sleep_t<std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<Rep, Period>>>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, sleep_t<std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<Rep, Period>>> arg)
    {
        if (!frame.steady_timer)
            frame.steady_timer = std::make_unique<boost::asio::steady_timer>(frame.get_executor());

        return detail::basic_timer_awaitable<promise_frame<Ts...>, boost::asio::steady_timer&> {
            frame, *frame.steady_timer, arg.expiry_time
        };
    }
};

template<class Clock, class Rep, class Period>
struct promise_awaitable<sleep_t<std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, sleep_t<std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>> arg)
    {
        return detail::timer_awaitable<promise_frame<Ts...>, Clock> { frame, arg.expiry_time };
    }
};

} // namespace detail
} // namespace webasio::coro
