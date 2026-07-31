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

template<class Rep, class Period>
constexpr auto sleep_for(std::chrono::duration<Rep, Period> duration)
{
    return sleep_t<std::chrono::duration<Rep, Period>> { duration };
}

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
        if (!frame.cached_steady_timer)
            frame.cached_steady_timer = std::make_unique<boost::asio::steady_timer>(frame.get_executor());

        return detail::basic_timer_awaitable<promise_frame<Ts...>, boost::asio::steady_timer&> {
            frame, *frame.cached_steady_timer, arg.duration
        };
    }
};

template<class Rep, class Period>
struct promise_awaitable<sleep_t<std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<Rep, Period>>>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, sleep_t<std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<Rep, Period>>> arg)
    {
        if (!frame.cached_steady_timer)
            frame.cached_steady_timer = std::make_unique<boost::asio::steady_timer>(frame.get_executor());

        return detail::basic_timer_awaitable<promise_frame<Ts...>, boost::asio::steady_timer&> {
            frame, *frame.cached_steady_timer, arg.expiry_time
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
