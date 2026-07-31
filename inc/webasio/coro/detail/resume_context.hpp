//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>

#include <atomic>
#include <coroutine>
#include <memory>


namespace webasio::coro::detail {

/**
 * @brief Race-free coordinator between an awaitable and its completion handler.
 * @internal
 * @details
 * A suspended awaitable and the async operation's completion handler may run
 * concurrently: the operation can complete before the awaiting coroutine has
 * finished suspending (eager completion), or the handler may be destroyed
 * without completing. `resume_context` uses a single atomic state machine so
 * exactly one side takes responsibility for resuming (or destroying) the
 * coroutine, and exposes the frame's executor/cancellation slot to the
 * handler through CRTP.
 *
 * @tparam Awaitable The CRTP-derived awaitable providing `frame()`/`set_value`.
 */
template<class Awaitable>
class resume_context {
public:
    /// Lifecycle states of the pending async operation.
    enum class state {
        initializing, /// awaitable is initializing the async operation
        awaiting,     /// awaitable is waiting for the async operation completion
        completed,    /// the async operation has completed
        destroyed,    /// the completion handler was destroyed without completing
    };

    // called by awaitable to enter state awaiting, returns true if the awaitable can resume
    bool resume_awaiting(std::coroutine_handle<> h) noexcept
    {
        switch (state_.exchange(state::awaiting)) {
        case state::completed:
            return true;
        case state::destroyed:
            h.destroy();
            [[fallthrough]];
        default:
            return false;
        }
    }

    // called by completion handler to enter state completed, returns true if the handler can resume
    bool resume_completed() noexcept
    {
        return state_.exchange(state::completed) == state::awaiting;
    }

    // called by completion handler to destroy the coroutine
    void destroy(std::coroutine_handle<> h) noexcept
    {
        if (state_.exchange(state::destroyed) == state::awaiting)
            h.destroy();
    }

    template<class... Args>
    void set_value(Args&&... args)
    {
        static_cast<Awaitable&>(*this).set_value(std::forward<Args>(args)...);
    }

    boost::asio::any_io_executor const& get_executor() const noexcept
    {
        return static_cast<Awaitable const&>(*this).frame().get_executor();
    }

    boost::asio::cancellation_slot get_cancellation_slot() const noexcept
    {
        return static_cast<Awaitable const&>(*this).frame().get_cancellation_slot();
    }

    bool restore_shared_this(std::weak_ptr<void const> weak_this) noexcept
    {
        return (static_cast<Awaitable&>(*this).frame().shared_this = weak_this.lock()) != nullptr;
    }

private:
    std::atomic<state> state_ { state::initializing };
};

} // webasio::coro::detail
