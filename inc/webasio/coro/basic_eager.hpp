//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/memory_cache.hpp>

#include <coroutine>


namespace webasio::coro {

struct basic_eager {
    struct promise_type {
        void* operator new(std::size_t size) { return memory_cache::tls::alloc(size); }
        void operator delete(void* ptr, std::size_t size) { memory_cache::tls::free(ptr, size); }
        basic_eager get_return_object() const noexcept { return {}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }
        void unhandled_exception() const { throw; }
        void return_void() const noexcept {}
    };
};

} // namespace webasio::coro
