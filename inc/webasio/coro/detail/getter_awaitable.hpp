//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <coroutine>
#include <utility>


namespace webasio::coro::detail {

/**
 * @brief Always-ready awaitable that yields a stored value on resume.
 * @internal
 * @details Used to expose synchronous coroutine-frame accessors (executor,
 * cancellation slot/status) through the `co_await` syntax without suspending.
 * @tparam T The type returned by `await_resume`.
 * @tparam U The stored value type (defaults to @p T).
 */
template<class T, class U = T>
struct getter_awaitable {
    U value;

    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(std::coroutine_handle<> h) const noexcept {}
    constexpr T await_resume() const noexcept(noexcept(T(std::declval<U>()))) { return value; }
};

} // namespace webasio::coro::detail
