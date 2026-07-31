//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/completion_handler.hpp>
#include <webasio/coro/outcome.hpp>

#include <boost/asio/deferred.hpp>


namespace webasio::coro::detail {

/**
 * @brief Strips a leading error argument from an Asio completion signature.
 * @internal
 * @details Given the completion arguments, drops a leading `std::error_code`,
 * `boost::system::error_code` or `std::exception_ptr` (which are surfaced as
 * exceptions) and yields the @ref outcome for the remaining value(s).
 */
template<class... Ts>
struct asio_outcome {
    using type = outcome<Ts...>;
};

template<class... Ts>
struct asio_outcome<std::error_code, Ts...> {
    using type = outcome<Ts...>;
};

template<class... Ts>
struct asio_outcome<boost::system::error_code, Ts...> {
    using type = outcome<Ts...>;
};

template<class... Ts>
struct asio_outcome<std::exception_ptr, Ts...> {
    using type = outcome<Ts...>;
};

template<class... Ts>
using asio_outcome_t = typename asio_outcome<Ts...>::type;

/**
 * @brief Awaitable adapting a raw Asio deferred operation to `co_await`.
 * @internal
 * @details Enables `co_await`ing a Boost.Asio async operation directly inside
 * a coroutine. A leading `error_code`/`exception_ptr` completion argument is
 * converted into a thrown exception (`std::system_error` for error codes);
 * the remaining arguments become the awaited value. Completion is coordinated
 * through @ref completion_handler / @ref resume_context to handle eager
 * completion and cancellation.
 *
 * @note Because errors are thrown, inspect them with try/catch rather than
 * destructuring a returned tuple.
 */
template<class Frame, class Signature, class Initiation, class... InitArgs>
class deferred_awaitable;

template<class Frame, class... Args, class Initiation, class... InitArgs>
class deferred_awaitable<Frame, void(Args...), Initiation, InitArgs...> : resume_context<deferred_awaitable<Frame, void(Args...), Initiation, InitArgs...>> {
    friend class resume_context<deferred_awaitable<Frame, void(Args...), Initiation, InitArgs...>>;

public:
    template<class Operation>
    deferred_awaitable(Frame& f, Operation&& op)
        : frame_ { f }
        , op_ { std::forward<Operation>(op) }
    {}

    template<class CArg, class... CArgs>
    void set_value(CArg&& arg, CArgs&&... args)
    {
        if constexpr (std::same_as<std::remove_cvref_t<CArg>, std::error_code>) {
            if (arg)
                result_.set_exception(std::system_error(std::forward<CArg>(arg)));
            else
                result_.set_value(std::forward<CArgs>(args)...);
        }
        else if constexpr (std::same_as<std::remove_cvref_t<CArg>, boost::system::error_code>) {
            if (arg)
                result_.set_exception(std::system_error(std::forward<CArg>(arg)));
            else
                result_.set_value(std::forward<CArgs>(args)...);
        }
        else if constexpr (std::same_as<std::remove_cvref_t<CArg>, std::exception_ptr>) {
            if (arg)
                result_.set_exception(arg);
            else
                result_.set_value(std::forward<CArgs>(args)...);
        }
        else
            result_.set_value(std::forward<CArg>(arg), std::forward<CArgs>(args)...);
    }

    void set_value()
    {}

    Frame& frame() { return frame_; }
    Frame const& frame() const { return frame_; }

    bool await_ready() const noexcept { return false; }

    template<class Caller>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Caller> h) noexcept
    {
        try {
            std::move(op_)(completion_handler { *this, h, frame_.shared_this });
        }
        catch (...) {
            result_.set_exception(std::current_exception());
            return h;
        }

        auto saved_shared_this = std::move(frame_.shared_this);
        if (this->resume_awaiting(h)) {
            frame_.shared_this = std::move(saved_shared_this);
            return h;
        }

        return std::noop_coroutine();
    }

    decltype(auto) await_resume()
    {
        return result_.get();
    }

private:
    Frame& frame_;
    asio_outcome_t<Args...> result_;
    boost::asio::deferred_async_operation<void(Args...), Initiation, InitArgs...> op_;
};

} // webasio::coro::detail
