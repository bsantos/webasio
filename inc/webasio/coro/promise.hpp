//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/promise_frame.hpp>

namespace webasio::coro {

template<class... Ts>
class [[nodiscard]] promise {
public:
    using promise_type = detail::promise_frame<Ts...>;

    auto get_awaitable() && noexcept
    {
        return std::exchange(m_coro, nullptr)->get_awaitable();
    }

private:
    template<class...>
    friend struct detail::promise_frame;
    friend struct detail::init_await;

    promise() = delete;
    promise(promise&&) = delete;
    promise(promise const&) = delete;
    promise& operator=(promise&&) = delete;
    promise& operator=(promise const&) = delete;

    promise(promise_type* coro)
        : m_coro { coro }
    {}

    unique_handle<promise_type> m_coro;
};


template<class T>
struct is_promise : std::false_type {};

template<class... Ts>
struct is_promise<promise<Ts...>> : std::true_type {};

template<class T>
inline constexpr bool is_promise_v = is_promise<T>::value;


template<class T>
struct promise_outcome;

template<class... Ts>
struct promise_outcome<promise<Ts...>> {
    using type = outcome<Ts...>;
};

template<class T>
using promise_outcome_t = typename promise_outcome<T>::type;

} // namespace webasio::coro

namespace webasio {

template<class... Ts>
using co_promise = coro::promise<Ts...>;

} // namespace webasio
