//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <webasio/coro/detail/promise_frame.hpp>


namespace webasio::coro {

struct detached;

} // namespace webasio::coro

namespace webasio::coro::detail {

template<>
struct promise_frame<detached> : detail::promise_frame<detail::promise_no_base_tag> {
    detached get_return_object() const noexcept;
    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }
    void unhandled_exception() const noexcept { std::rethrow_exception(std::current_exception()); }
    void return_void() const noexcept {}
};

} // namespace webasio::coro::detail
