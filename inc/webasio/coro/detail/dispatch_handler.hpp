//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/unique_handle.hpp>
#include <webasio/memory_cache.hpp>


namespace webasio::coro::detail {

/**
 * @brief Handler that resumes a coroutine when run by an executor.
 * @internal
 * @details Posted or dispatched to an executor to resume (or, on the
 * @ref resume_context path, safely destroy) a coroutine on the right thread.
 * The @p Awaitable overload cooperates with @ref resume_context for the
 * executor-hop-on-completion case; the `void` specialization is an
 * unconditional resume used for plain post/dispatch hops. Allocates from the
 * thread-local cache.
 * @tparam Awaitable The awaitable to coordinate with, or `void` for a plain
 * resume.
 */
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

/// @internal Unconditional-resume specialization for plain executor hops.
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
