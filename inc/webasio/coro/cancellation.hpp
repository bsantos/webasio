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

using cancellation_type = boost::asio::cancellation_type;

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
    void emit(cancellation_type type = cancellation_type::terminal)
    {
        base::emit(type);
    }

    template<class Executor>
    void emit(Executor&& ex, cancellation_type type = cancellation_type::terminal)
    {
        boost::asio::dispatch(std::forward<Executor>(ex), emit_signal { this, type });
    }
};

class multicast_cancellation {
    auto get()
    {
        if (!signals_)
            signals_ = std::make_shared<std::vector<signal*>>();

        return signals_;
    }

public:
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
    void emit(cancellation_type type = cancellation_type::terminal)
    {
        if (!signals_)
            return;

        for (auto& sig : *signals_)
            sig->emit(type);
    }

    template<class Executor>
    void emit(Executor&& ex, cancellation_type type = cancellation_type::terminal)
    {
        boost::asio::dispatch(std::forward<Executor>(ex), emit_signal { type, signals_ });
    }

private:
    std::shared_ptr<std::vector<signal*>> signals_;
};

using multicast_cancellation_signal = multicast_cancellation::signal;


struct cancelled_t {};
struct set_cancellation_slot_t { boost::asio::cancellation_slot slot; };
struct cancellation_slot_t {
    constexpr set_cancellation_slot_t operator()(boost::asio::cancellation_slot slot) const noexcept { return { slot }; }
};

inline constexpr cancelled_t cancelled;
inline constexpr cancellation_slot_t cancellation_slot;


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

template<>
struct reset_cancellation_state_t<> {
    constexpr auto operator()() const noexcept
    {
        return reset_cancellation_state_t<void> {};
    }

    constexpr auto operator()(boost::asio::cancellation_slot slot) const noexcept
    {
        return reset_cancellation_state_t<set_cancellation_slot_t> { slot };
    }

    template<class Filter>
    constexpr auto operator()(Filter&& filter) const noexcept
    {
        return reset_cancellation_state_t<void, std::decay_t<Filter>> { std::forward<Filter>(filter) };
    }

    template<class Filter>
    constexpr auto operator()(boost::asio::cancellation_slot slot, Filter&& filter) const noexcept
    {
        return reset_cancellation_state_t<set_cancellation_slot_t, std::decay_t<Filter>> { slot, std::forward<Filter>(filter) };
    }

    template<class InFilter, class OutFilter>
    constexpr auto operator()(InFilter&& in_filter, OutFilter&& out_filter) const noexcept
    {
        return reset_cancellation_state_t<void, std::decay_t<InFilter>, std::decay_t<OutFilter>> { std::forward<InFilter>(in_filter), std::forward<OutFilter>(out_filter) };
    }

    template<class InFilter, class OutFilter>
    constexpr auto operator()(boost::asio::cancellation_slot slot, InFilter&& in_filter, OutFilter&& out_filter) const noexcept
    {
        return reset_cancellation_state_t<set_cancellation_slot_t, std::decay_t<InFilter>, std::decay_t<OutFilter>> { slot, std::forward<InFilter>(in_filter), std::forward<OutFilter>(out_filter) };
    }
};

inline constexpr reset_cancellation_state_t<> reset_cancellation_state;

namespace detail {

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
