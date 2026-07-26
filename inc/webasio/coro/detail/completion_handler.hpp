#pragma once

#include <webasio/coro/unique_handle.hpp>
#include <webasio/memory_cache.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_immediate_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>


namespace webasio::coro::detail {

template<class Frame, class Awaitable>
struct completion_handler {
    using shared_this_t = std::shared_ptr<void const>;
    using weak_this_t = std::weak_ptr<void const>;

    Frame& frame_;
    Awaitable& awaitable_;
    unique_handle<> handle_;
    weak_this_t weak_this_;
    bool restore_shared_this_;

    completion_handler(Frame& f, Awaitable& aw, std::coroutine_handle<> h)
        : frame_ { f }
        , awaitable_ { aw }
        , handle_ { h }
        , weak_this_ { f.shared_this }
        , restore_shared_this_ { f.shared_this }
    {}

    completion_handler(Frame& f, Awaitable& aw, unique_handle<> h)
        : frame_ { f }
        , awaitable_ { aw }
        , handle_ { std::move(h) }
    {}

    using cancellation_slot_type = boost::asio::cancellation_slot;

    boost::asio::cancellation_slot get_cancellation_slot() const noexcept
    {
        return frame_.get_cancellation_slot();
    }

    using allocator_type = cached_tls_allocator<>;

    allocator_type get_allocator() const noexcept
    {
        return {};
    }

    template<class... Args>
    void operator()(Args&&... args)
    {
        awaitable_.set_value(std::forward<Args>(args)...);

        if (restore_shared_this_) {
            frame_.shared_this = weak_this_.lock();
            if (!frame_.shared_this)
                return;
        }

        handle_.resume();
    }
};

} // webasio::coro::detail

namespace boost::asio {

template<class Frame, class Awaitable, class Candidate>
struct associated_executor<::webasio::coro::detail::completion_handler<Frame, Awaitable>, Candidate> {
    using type = any_io_executor;

    static type get(const ::webasio::coro::detail::completion_handler<Frame, Awaitable>& handler,
                    const Candidate& candidate = Candidate()) noexcept
    {
        if (auto const& executor = handler.frame_.get_executor())
            return executor;

        return any_io_executor { std::nothrow, candidate };
    }
};

template<class Frame, class Awaitable, class Candidate>
struct associated_immediate_executor<::webasio::coro::detail::completion_handler<Frame, Awaitable>, Candidate> {
    using type = any_io_executor;

    static type get(const ::webasio::coro::detail::completion_handler<Frame, Awaitable>& handler,
                    const Candidate& candidate = Candidate()) noexcept
    {
        if (auto const& executor = handler.frame_.get_executor())
            return executor;

        return any_io_executor { std::nothrow, candidate };
    }
};

} // namespace boost::asio
