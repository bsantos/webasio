#pragma once

#include <webasio/coro/detail/completion_handler.hpp>
#include <webasio/coro/outcome.hpp>

#include <boost/asio/deferred.hpp>


namespace webasio::coro::detail {

template<class Signature, class Initiation, class... InitArgs>
struct deferred_awaitable;

template<class... Args, class Initiation, class... InitArgs>
struct deferred_awaitable<void(Args...), Initiation, InitArgs...> {
    boost::asio::deferred_async_operation<void(Args...), Initiation, InitArgs...> op_;
    outcome<Args...> result_ {};

    template<class... CArgs>
    void set_value(CArgs&&... args)
    {
        result_.set_value(std::forward<CArgs>(args)...);
    }

    bool await_ready() const noexcept { return false; }

    template<class Frame>
    void await_suspend(std::coroutine_handle<Frame> h) noexcept
    {
        std::move(op_)(completion_handler { h.promise(), *this, h, });
    }

    decltype(auto) await_resume()
    {
        return result_.get();
    }
};

} // webasio::coro::detail
