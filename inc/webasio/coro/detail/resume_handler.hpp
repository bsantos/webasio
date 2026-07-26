#pragma once

#include <webasio/coro/unique_handle.hpp>
#include <webasio/memory_cache.hpp>

namespace webasio::coro::detail {

struct resume_handler {
    unique_handle<> handle_;

    resume_handler(std::coroutine_handle<> h)
        : handle_ { h }
    {}

    resume_handler(unique_handle<> h)
        : handle_ { std::move(h) }
    {}

    using allocator_type = cached_tls_allocator<>;

    allocator_type get_allocator() const noexcept
    {
        return {};
    }

    void operator()()
    {
        handle_.resume();
    }
};

} // webasio::coro::detail
