#pragma once

#include <webasio/coro/detail/completion_handler.hpp>

#include <boost/asio/basic_waitable_timer.hpp>


namespace webasio::coro::detail {

template<class Frame, class Timer>
class basic_timer_awaitable : resume_context<basic_timer_awaitable<Frame, Timer>> {
    using timer_type = std::remove_cvref_t<Timer>;
    using duration_type = timer_type::duration;
    using time_point_type = timer_type::time_point;
    using clock_type = timer_type::clock_type;

    friend class resume_context<basic_timer_awaitable<Frame, Timer>>;

public:
    template<class Arg>
    basic_timer_awaitable(Frame& f, Arg&& arg, duration_type expiry)
        : frame_ { f }
        , timer_ { std::forward<Arg>(arg) }
    {
        timer_.expires_after(expiry);
    }

    template<class Arg>
    basic_timer_awaitable(Frame& f, Arg&& arg, time_point_type expiry)
        : frame_ { f }
        , timer_ { std::forward<Arg>(arg) }
    {
        timer_.expires_at(expiry);
    }

    template<class DurationOrTimePoint>
    basic_timer_awaitable(Frame& f, DurationOrTimePoint expirity)
        : basic_timer_awaitable { f, f.get_executor(), expirity }
    {}


    Frame& frame() { return frame_; }
    Frame const& frame() const { return frame_; }

    void set_value(boost::system::error_code ec)
    {
        ec_ = ec;
    }

    bool await_ready() const noexcept
    {
        return timer_.expiry() <= clock_type::now();
    }

    template<class Caller>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Caller> h) noexcept
    {
        timer_.async_wait(completion_handler { *this, h, frame_.shared_this });

        auto saved_shared_this = std::move(frame_.shared_this);
        if (this->resume_awaiting(h)) {
            frame_.shared_this = std::move(saved_shared_this);
            return h;
        }

        return std::noop_coroutine();
    }

    auto await_resume() const noexcept
    {
        return std::make_tuple(ec_, timer_.expiry());
    }

private:
    Frame& frame_;
    Timer timer_;
    boost::system::error_code ec_;
};

template<class Frame, class Clock>
using timer_awaitable = basic_timer_awaitable<Frame, boost::asio::basic_waitable_timer<Clock>>;

} // webasio::coro::detail
