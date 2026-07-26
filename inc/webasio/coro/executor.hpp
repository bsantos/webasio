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

struct executor_t { };

inline constexpr executor_t executor;

namespace detail {

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
