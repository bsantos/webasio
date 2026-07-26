#pragma once

#include <webasio/coro/detail/completion_handler.hpp>

#include <boost/asio/basic_waitable_timer.hpp>


namespace webasio::coro::detail {

template<class Timer>
class basic_timer_awaitable {
    using timer_type = std::remove_cvref_t<Timer>;

public:
    using duration_type = timer_type::duration;
    using time_point_type = timer_type::time_point;

    template<class Arg>
    basic_timer_awaitable(Arg&& arg, duration_type expiry)
        : timer_ { std::forward<Arg>(arg) }
    {
        timer_.expires_after(expiry);
    }

    template<class Arg>
    basic_timer_awaitable(Arg&& arg, time_point_type expiry)
        : timer_ { std::forward<Arg>(arg) }
    {
        timer_.expires_at(expiry);
    }

    void set_value(boost::system::error_code ec)
    {
        ec_ = ec;
    }

    bool await_ready() noexcept
    {
        if (timer_.expiry() <= time_point_type { duration_type::zero() }) {
            ec_ = boost::asio::error::make_error_code(boost::asio::error::invalid_argument);
            return true;
        }

        return false;
    }

    template<class Frame>
    void await_suspend(std::coroutine_handle<Frame> h) noexcept
    {
        timer_.async_wait(completion_handler { h.promise(), *this, h });
    }

    auto await_resume(std::source_location loc = std::source_location::current())
    {
        if (ec_ == boost::asio::error::operation_aborted)
            return time_point_type::min();

        if (ec_)
            throw boost::system::system_error(ec_, loc.function_name());

        return timer_.expiry();
    }

private:
    Timer                     timer_;
    boost::system::error_code ec_;
};

template<class Clock>
using timer_awaitable = basic_timer_awaitable<boost::asio::basic_waitable_timer<Clock>>;

} // webasio::coro::detail
