//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/assert.hpp>
#include <webasio/coro/detail/getter_awaitable.hpp>
#include <webasio/coro/detail/promise_frame.hpp>
#include <webasio/memory_cache.hpp>

#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/dispatch.hpp>


namespace webasio::coro {

/// Re-export of `boost::asio::cancellation_type`.
using cancellation_type = boost::asio::cancellation_type;

/**
 * @brief A cancellation signal that can emit from an arbitrary executor.
 *
 * @details
 * Extends `boost::asio::cancellation_signal` with an executor-aware
 * `emit(executor, type)` overload that marshals the emission onto the given
 * executor via `boost::asio::dispatch`. This lets code running on one thread
 * safely cancel work whose slot is serviced on another (for example the
 * strand owning the coroutine). Emitted work is allocated from the
 * thread-local cache.
 *
 * @see webasio::coro::multicast_cancellation
 * @see webasio::coro::reset_cancellation_state
 */
class cancellation_signal : public boost::asio::cancellation_signal {
    using base = boost::asio::cancellation_signal;

    struct emit_signal {
        cancellation_signal* signal_;
        cancellation_type type_;

        using allocator_type = cached_tls_allocator<>;

        allocator_type get_allocator() const noexcept
        {
            return {};
        }

        void operator()() const
        {
            signal_->emit(type_);
        }
    };

public:
    /// Emits @p type synchronously to the connected slot.
    void emit(cancellation_type type = cancellation_type::terminal)
    {
        base::emit(type);
    }

    /// Emits @p type by dispatching the emission onto @p ex.
    template<class Executor>
    void emit(Executor&& ex, cancellation_type type = cancellation_type::terminal)
    {
        boost::asio::dispatch(std::forward<Executor>(ex), emit_signal { this, type });
    }
};

/**
 * @brief Fan-out cancellation source driving many child signals at once.
 *
 * @details
 * Maintains a shared registry of @ref signal instances. Emitting on the
 * `multicast_cancellation` forwards the cancellation type to every currently
 * registered child signal, allowing a single trigger to cancel a group of
 * coroutines. Each child registers itself on construction and deregisters on
 * destruction, so the group membership tracks child lifetimes automatically.
 * An executor-aware `emit` overload is provided for cross-thread use.
 *
 * @see webasio::coro::multicast_cancellation_signal
 */
class multicast_cancellation {
    auto get()
    {
        if (!signals_)
            signals_ = std::make_shared<std::vector<signal*>>();

        return signals_;
    }

public:
    /**
     * @brief A per-task child signal belonging to a @ref multicast_cancellation.
     * @details Registers with the parent group on construction and removes
     * itself on destruction. Cancel the whole group through the parent.
     */
    class signal : public cancellation_signal {
    public:
        signal(multicast_cancellation& multicast)
            : signals_ { multicast.get() }
        {
            signals_->push_back(this);
        }

        ~signal()
        {
            std::erase(*signals_, this);
        }

    private:
        std::shared_ptr<std::vector<signal*>> signals_;
    };

private:
    struct emit_signal {
        cancellation_type type_;
        std::shared_ptr<std::vector<signal*>> signals_;

        using allocator_type = cached_tls_allocator<>;

        allocator_type get_allocator() const noexcept
        {
            return {};
        }

        void operator()() const
        {
            if (!signals_)
                return;

            for (auto& sig : *signals_)
                sig->emit(type_);
        }
    };

public:
    /// Emits @p type synchronously to every registered child signal.
    void emit(cancellation_type type = cancellation_type::terminal)
    {
        if (!signals_)
            return;

        for (auto& sig : *signals_)
            sig->emit(type);
    }

    /// Emits @p type to every child by dispatching the emission onto @p ex.
    template<class Executor>
    void emit(Executor&& ex, cancellation_type type = cancellation_type::terminal)
    {
        boost::asio::dispatch(std::forward<Executor>(ex), emit_signal { type, signals_ });
    }

private:
    std::shared_ptr<std::vector<signal*>> signals_;
};

/// A child signal of a @ref multicast_cancellation group.
using multicast_cancellation_signal = multicast_cancellation::signal;


/// Tag for the @ref cancelled awaitable. @internal
struct cancelled_t {};
/// Tag carrying a slot for @ref cancellation_slot / reset. @internal
struct set_cancellation_slot_t { boost::asio::cancellation_slot slot; };
/// Factory tag producing a @ref set_cancellation_slot_t from a slot. @internal
struct cancellation_slot_t {
    constexpr set_cancellation_slot_t operator()(boost::asio::cancellation_slot slot) const noexcept { return { slot }; }
};

/**
 * @brief Awaitable yielding the coroutine's current cancellation status.
 * @details `co_await cancelled` returns the
 * `boost::asio::cancellation_type` accumulated in the coroutine's
 * cancellation state (`none` if not cancelled).
 */
inline constexpr cancelled_t cancelled;

/**
 * @brief Awaitable yielding or setting the coroutine's cancellation slot.
 * @details `co_await cancellation_slot` yields the current slot;
 * `co_await cancellation_slot(slot)` installs @p slot and resets state.
 */
inline constexpr cancellation_slot_t cancellation_slot;


/**
 * @brief Tag family describing a cancellation-state reset request.
 *
 * @details
 * Specializations encode the optional combination of a new slot, an input
 * filter and an output filter to install. Construct instances through the
 * @ref reset_cancellation_state callable rather than directly.
 *
 * @tparam Options Encoded reset options (slot presence and filter types).
 */
template<class... Options>
struct reset_cancellation_state_t;

template<>
struct reset_cancellation_state_t<void> {};

template<class Filter>
struct reset_cancellation_state_t<Filter> {
    Filter filter;
};

template<class InFilter, class OutFilter>
struct reset_cancellation_state_t<InFilter, OutFilter> {
    InFilter in_filter;
    OutFilter out_filter;
};

template<>
struct reset_cancellation_state_t<set_cancellation_slot_t> {
    boost::asio::cancellation_slot slot;
};

template<class Filter>
struct reset_cancellation_state_t<set_cancellation_slot_t, Filter> {
    boost::asio::cancellation_slot slot;
    Filter filter;
};

template<class InFilter, class OutFilter>
struct reset_cancellation_state_t<set_cancellation_slot_t, InFilter, OutFilter> {
    boost::asio::cancellation_slot slot;
    InFilter in_filter;
    OutFilter out_filter;
};

/**
 * @brief Builder for @ref reset_cancellation_state_t reset requests.
 *
 * @details Each `operator()` overload produces an awaitable that, when
 * `co_await`ed, (re)initializes the coroutine's cancellation state. Overloads
 * accept an optional new slot plus an optional input filter and output
 * filter. Establish cancellation state with this before awaiting cancellable
 * work so that later `co_await cancelled` reflects the intended semantics.
 */
template<>
struct reset_cancellation_state_t<> {
    /// Resets state, keeping the current slot and clearing filters.
    constexpr auto operator()() const noexcept
    {
        return reset_cancellation_state_t<void> {};
    }

    /// Resets state and installs a new @p slot.
    constexpr auto operator()(boost::asio::cancellation_slot slot) const noexcept
    {
        return reset_cancellation_state_t<set_cancellation_slot_t> { slot };
    }

    /// Resets state with a single @p filter (used as both in and out filter).
    template<class Filter>
    constexpr auto operator()(Filter&& filter) const noexcept
    {
        return reset_cancellation_state_t<void, std::decay_t<Filter>> { std::forward<Filter>(filter) };
    }

    /// Resets state with a new @p slot and a single @p filter.
    template<class Filter>
    constexpr auto operator()(boost::asio::cancellation_slot slot, Filter&& filter) const noexcept
    {
        return reset_cancellation_state_t<set_cancellation_slot_t, std::decay_t<Filter>> { slot, std::forward<Filter>(filter) };
    }

    /// Resets state with distinct @p in_filter and @p out_filter.
    template<class InFilter, class OutFilter>
    constexpr auto operator()(InFilter&& in_filter, OutFilter&& out_filter) const noexcept
    {
        return reset_cancellation_state_t<void, std::decay_t<InFilter>, std::decay_t<OutFilter>> { std::forward<InFilter>(in_filter), std::forward<OutFilter>(out_filter) };
    }

    /// Resets state with a new @p slot and distinct in/out filters.
    template<class InFilter, class OutFilter>
    constexpr auto operator()(boost::asio::cancellation_slot slot, InFilter&& in_filter, OutFilter&& out_filter) const noexcept
    {
        return reset_cancellation_state_t<set_cancellation_slot_t, std::decay_t<InFilter>, std::decay_t<OutFilter>> { slot, std::forward<InFilter>(in_filter), std::forward<OutFilter>(out_filter) };
    }
};

/**
 * @brief Establishes or replaces the coroutine's cancellation state.
 * @details Await the result of one of its call operators, e.g.
 * `co_await reset_cancellation_state(slot)`.
 * @see webasio::coro::cancelled
 * @see webasio::coro::cancellation_slot
 */
inline constexpr reset_cancellation_state_t<> reset_cancellation_state;

namespace detail {

// Maps the cancellation tag types above to their concrete awaitables and
// applies the requested changes to the coroutine frame's cancellation state. @internal
template<>
struct promise_awaitable<cancelled_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, cancelled_t) noexcept
    {
        return detail::getter_awaitable<boost::asio::cancellation_type> { frame.cancellation_state.cancelled() };
    }
};

template<>
struct promise_awaitable<cancellation_slot_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, cancellation_slot_t) noexcept
    {
        return detail::getter_awaitable<boost::asio::cancellation_slot> { frame.get_cancellation_slot() };
    }
};

template<>
struct promise_awaitable<set_cancellation_slot_t> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, set_cancellation_slot_t arg)
    {
        frame.cancellation_slot = arg.slot;
        frame.cancellation_state = boost::asio::cancellation_state {};
        return std::suspend_never { };
    }
};

template<>
struct promise_awaitable<reset_cancellation_state_t<void>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, reset_cancellation_state_t<void>)
    {
        frame.cancellation_state = boost::asio::cancellation_state { frame.cancellation_slot };
        return std::suspend_never { };
    }
};

template<>
struct promise_awaitable<reset_cancellation_state_t<set_cancellation_slot_t>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, reset_cancellation_state_t<set_cancellation_slot_t> arg)
    {
        frame.cancellation_slot = arg.slot;
        frame.cancellation_state = boost::asio::cancellation_state { frame.cancellation_slot };
        return std::suspend_never { };
    }
};

template<class Filter>
struct promise_awaitable<reset_cancellation_state_t<void, Filter>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, reset_cancellation_state_t<void, Filter> arg)
    {
        frame.cancellation_state = boost::asio::cancellation_state { frame.cancellation_slot, std::move(arg.filter) };
        return std::suspend_never { };
    }
};

template<class Filter>
struct promise_awaitable<reset_cancellation_state_t<set_cancellation_slot_t, Filter>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, reset_cancellation_state_t<set_cancellation_slot_t, Filter> arg)
    {
        frame.cancellation_slot = arg.slot;
        frame.cancellation_state = boost::asio::cancellation_state { frame.cancellation_slot, std::move(arg.filter) };
        return std::suspend_never { };
    }
};

template<class InFilter, class OutFilter>
struct promise_awaitable<reset_cancellation_state_t<void, InFilter, OutFilter>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, reset_cancellation_state_t<void, InFilter, OutFilter> arg)
    {
        frame.cancellation_state = boost::asio::cancellation_state { frame.cancellation_slot, std::move(arg.in_filter), std::move(arg.out_filter) };
        return std::suspend_never { };
    }
};

template<class InFilter, class OutFilter>
struct promise_awaitable<reset_cancellation_state_t<set_cancellation_slot_t, InFilter, OutFilter>> {
    template<class... Ts>
    static auto get(promise_frame<Ts...>& frame, reset_cancellation_state_t<set_cancellation_slot_t, InFilter, OutFilter> arg)
    {
        frame.cancellation_slot = arg.slot;
        frame.cancellation_state = boost::asio::cancellation_state { frame.cancellation_slot, std::move(arg.in_filter), std::move(arg.out_filter) };
        return std::suspend_never { };
    }
};

} // namespace detail
} // namespace webasio::coro
