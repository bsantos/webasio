//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/await.hpp>
#include <webasio/coro/promise.hpp>

#include <boost/asio/async_result.hpp>
#include <boost/asio/deferred.hpp>


namespace webasio::coro {

template<class... Ts,
         boost::asio::completion_token_for<void(promise_outcome_t<promise<Ts...>>)> CompletionToken = boost::asio::deferred_t>
inline decltype(auto) await(promise<Ts...> coro, CompletionToken&& token = boost::asio::deferred_t {})
{
    return boost::asio::async_initiate<CompletionToken, void(promise_outcome_t<promise<Ts...>>)>(
        detail::init_await { },
        token,
        detail::init_await::get_handle(std::move(coro))
    );
}

template<class T>
using await_completion_signature_t = void(promise_outcome_t<std::invoke_result_t<T>>);

template<class T>
concept await_function = is_promise_v<std::invoke_result_t<std::decay_t<T>>>;

template<await_function Function,
         boost::asio::completion_token_for<await_completion_signature_t<std::decay_t<Function>>> CompletionToken = boost::asio::deferred_t>
inline decltype(auto) await(Function&& func, CompletionToken&& token = boost::asio::deferred_t {})
{
    return boost::asio::async_initiate<CompletionToken, await_completion_signature_t<std::decay_t<Function>>>(
        detail::init_await { },
        token,
        std::forward<Function>(func)
    );
}

} // namespace webasio::coro
