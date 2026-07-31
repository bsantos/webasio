//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>

#include <chrono>


namespace webasio::this_coro {

struct executor_t { };

inline constexpr executor_t executor;


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

} // namespace webasio::this_coro
