//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/resume_context.hpp>
#include <webasio/coro/unique_handle.hpp>
#include <webasio/memory_cache.hpp>

#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_immediate_executor.hpp>
#include <boost/asio/inline_or_executor.hpp>


namespace webasio::coro::detail {

/**
 * @brief Asio completion handler that resumes a suspended coroutine.
 * @internal
 * @details
 * Bound to an async operation initiated by an awaitable; on completion it
 * stores the result into the awaitable and resumes the coroutine, coordinating
 * with @ref resume_context to handle eager completion and cancellation races.
 * It forwards the coroutine's associated executor and cancellation slot to
 * Asio, allocates from the thread-local cache, and holds a weak reference to
 * the owning object (`shared_this`) which it re-locks before resuming, so a
 * concurrently destroyed owner aborts the coroutine safely. If the handler is
 * destroyed without being invoked, the coroutine is destroyed.
 *
 * @tparam Awaitable The awaitable that initiated the operation.
 */
template<class Awaitable>
class completion_handler {
    using shared_this_t = std::shared_ptr<void const>;
    using weak_this_t = std::weak_ptr<void const>;

public:
    completion_handler(resume_context<Awaitable>& ctx, std::coroutine_handle<> h, shared_this_t& shared_this)
        : context_ { ctx }
        , handle_ { h }
        , weak_this_ { shared_this }
        , restore_shared_this_ { shared_this }
    {}

    ~completion_handler()
    {
        // we can only destroy the coroutine if the awaitable allows it
        if (handle_)
            context_.destroy(handle_.release());
    }

    completion_handler(completion_handler&&) = default;
    completion_handler& operator=(completion_handler&&) = default;

    using cancellation_slot_type = boost::asio::cancellation_slot;

    boost::asio::cancellation_slot get_cancellation_slot() const noexcept
    {
        return context_.get_cancellation_slot();
    }

    using allocator_type = cached_tls_allocator<>;

    allocator_type get_allocator() const noexcept
    {
        return {};
    }

    boost::asio::any_io_executor const& executor() const noexcept
    {
        return context_.get_executor();
    }

    template<class... Args>
    void operator()(Args&&... args)
    {
        context_.set_value(std::forward<Args>(args)...);

        if (auto h = resume())
            h.resume();
    }

private:
    [[nodiscard]] std::coroutine_handle<> resume() noexcept
    {
        auto h = handle_.release();

        // in case of eager completion, the awaitable will resume the coroutine
        if (!context_.resume_completed())
            return nullptr;

        // if we fail to restore the shared_this, the coroutine must be destroyed
        if (restore_shared_this_ && !context_.restore_shared_this(std::move(weak_this_))) {
            h.destroy();
            return nullptr;
        }

        return h;
    }

private:
    resume_context<Awaitable>& context_;
    unique_handle<> handle_;
    weak_this_t weak_this_;
    bool restore_shared_this_;

};

} // webasio::coro::detail

namespace boost::asio {

template<class Awaitable, class Candidate>
struct associated_executor<webasio::coro::detail::completion_handler<Awaitable>, Candidate> {
    using type = any_io_executor;

    static type get(webasio::coro::detail::completion_handler<Awaitable> const& handler,
                    Candidate const& candidate = Candidate()) noexcept
    {
        if (auto const& executor = handler.executor())
            return executor;

        return any_io_executor { std::nothrow, candidate };
    }
};

template<class Awaitable, class Candidate>
struct associated_immediate_executor<webasio::coro::detail::completion_handler<Awaitable>, Candidate> {
    using type = inline_or_executor<Candidate>;

    static type get(webasio::coro::detail::completion_handler<Awaitable> const& handler,
                    Candidate const& candidate = Candidate()) noexcept
    {
        if (auto const& executor = handler.executor())
            return type { executor };

        return type { candidate };
    }
};

} // namespace boost::asio
