//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/detached_frame.hpp>

namespace webasio::coro {

/**
 * @brief Return type for a fire-and-forget coroutine.
 *
 * @details
 * A coroutine declared to return `detached` starts eagerly and runs to
 * completion independently; it produces no awaitable result. An exception
 * escaping a detached coroutine calls `std::terminate` (its
 * `unhandled_exception` is `noexcept` and rethrows), so detached workflows
 * must catch and handle their own errors at intended boundaries.
 *
 * @note Prefer the `webasio::co_detached` alias in user code.
 * @see webasio::coro::promise
 */
struct detached {
    using promise_type = detail::promise_frame<detached>;
};

inline detached detail::promise_frame<detached>::get_return_object() const noexcept { return {}; }

} // namespace webasio::coro

namespace webasio {

/// Public alias for @ref webasio::coro::detached.
using co_detached = coro::detached;

} // namespace webasio
