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

/**
 * @brief Adapts a coroutine `promise` into an Asio async operation.
 *
 * @details
 * Bridges a lazy @ref promise into the Boost.Asio universal asynchronous
 * model: the coroutine is started and its result is delivered to the
 * supplied completion token with the signature
 * `void(promise_outcome_t<promise<Ts...>>)`. The single completion argument
 * is an @ref outcome that carries either the produced values or a captured
 * exception; use `outcome::get()` to extract the value (re-throwing on
 * error).
 *
 * With the default `boost::asio::deferred` token the call returns a deferred
 * operation that starts the coroutine only when itself awaited or otherwise
 * initiated.
 *
 * @tparam Ts The value types carried by the promise.
 * @tparam CompletionToken An Asio completion token accepting the outcome.
 * @param coro The coroutine promise to run. Consumed by the call.
 * @param token The completion token invoked with the coroutine outcome.
 * @return Whatever the completion token's async_result produces.
 *
 * @see webasio::coro::promise
 * @see webasio::coro::outcome
 */
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

/// Completion signature produced by awaiting the result of a nullary @p T.
template<class T>
using await_completion_signature_t = void(promise_outcome_t<std::invoke_result_t<T>>);

/// Concept: a nullary callable whose result is a @ref promise.
template<class T>
concept await_function = is_promise_v<std::invoke_result_t<std::decay_t<T>>>;

/**
 * @brief Adapts a coroutine factory into an Asio async operation.
 *
 * @details
 * Deferred overload accepting a nullary callable that returns a @ref promise
 * (for example a lambda wrapping a coroutine call). The callable is invoked
 * to create the coroutine at initiation time, which is preferable when the
 * operation may be started lazily or repeatedly by the completion token.
 *
 * @tparam Function A callable satisfying @ref await_function.
 * @tparam CompletionToken An Asio completion token accepting the outcome.
 * @param func Factory invoked to produce the coroutine to run.
 * @param token The completion token invoked with the coroutine outcome.
 * @return Whatever the completion token's async_result produces.
 */
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
