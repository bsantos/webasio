#pragma once

#include <webasio/coro/detail/completion_handler.hpp>
#include <webasio/coro/outcome.hpp>

#include <boost/asio/deferred.hpp>


namespace webasio::coro::detail {

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

    template<class... CArgs>
    void set_value(CArgs&&... args)
    {
        result_.set_value(std::forward<CArgs>(args)...);
    }

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
    outcome<Args...> result_;
    boost::asio::deferred_async_operation<void(Args...), Initiation, InitArgs...> op_;
};

} // webasio::coro::detail
