//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/memory_cache.hpp>

#include <coroutine>


namespace webasio::coro::detail {

/**
 * @brief Minimal eager coroutine return type used to drive internal tasks.
 *
 * @details
 * `basic_detached` is a lightweight building block, not a user-facing API. Its
 * promise starts the coroutine immediately (`initial_suspend` and
 * `final_suspend` both never suspend) and does not retain the coroutine
 * frame, so the body must own its own lifetime. Frames are allocated from
 * the thread-local @ref webasio::memory_cache to avoid per-task heap
 * traffic. `unhandled_exception` is `noexcept` and rethrows, so an escaping
 * exception calls `std::terminate`.
 *
 * It is used, for example, to kick off the trampoline coroutine that wires a
 * @ref promise into an Asio completion handler.
 *
 * @internal
 */
struct basic_detached {
    struct promise_type {
        void* operator new(std::size_t size) { return memory_cache::tls::alloc(size); }
        void operator delete(void* ptr, std::size_t size) { memory_cache::tls::free(ptr, size); }
        basic_detached get_return_object() const noexcept { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void unhandled_exception() const noexcept { std::rethrow_exception(std::current_exception()); }
        void return_void() const noexcept {}
    };
};

} // namespace webasio::coro::detail
