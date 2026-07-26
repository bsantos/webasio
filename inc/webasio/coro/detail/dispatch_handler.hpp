#pragma once

#include <webasio/coro/unique_handle.hpp>
#include <webasio/memory_cache.hpp>


namespace webasio::coro::detail {

template<class Awaitable = void>
class dispatch_handler {
public:
    dispatch_handler(resume_context<Awaitable>& ctx, std::coroutine_handle<> h)
        : context_ { ctx }
        , handle_ { h }
    {}

    dispatch_handler(resume_context<Awaitable>& ctx, unique_handle<> h)
        : context_ { ctx }
        , handle_ { std::move(h) }
    {}

    dispatch_handler(dispatch_handler&&) = default;
    dispatch_handler& operator=(dispatch_handler&&) = default;

    ~dispatch_handler()
    {
        // we can only destroy the coroutine if the awaitable allows it
        if (handle_)
            context_.destroy(handle_.release());
    }

    using allocator_type = cached_tls_allocator<>;

    allocator_type get_allocator() const noexcept
    {
        return {};
    }

    void operator()()
    {
        auto h = handle_.release();

        if (context_.resume_completed())
            h.resume();
    }

private:
    resume_context<Awaitable>& context_;
    unique_handle<> handle_;
};

template<>
class dispatch_handler<void> {
public:
    dispatch_handler(unique_handle<> h)
        : handle_ { std::move(h) }
    {}

    dispatch_handler(std::coroutine_handle<> h)
        : handle_ { h }
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

private:
    unique_handle<> handle_;
};

} // webasio::coro::detail
