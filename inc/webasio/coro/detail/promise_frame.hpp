//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/assert.hpp>
#include <webasio/coro/detail/deferred_awaitable.hpp>
#include <webasio/coro/detail/getter_awaitable.hpp>
#include <webasio/coro/detail/resume_handler.hpp>
#include <webasio/coro/detail/timer_awaitable.hpp>
#include <webasio/coro/this_coro.hpp>
#include <webasio/memory_cache.hpp>

#include <boost/asio/cancellation_state.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>


namespace webasio::coro {

template<class... Ts>
class promise;

namespace detail {

template<class... Ts>
struct promise_frame_base {
    using value_type = typename outcome<Ts...>::value_type;

    outcome<Ts...> result;

    void return_value(value_type&& v) { result.set_value(std::move(v)); }
    void return_value(value_type const& v) { result.set_value(v); }
    void unhandled_exception() noexcept { result.set_exception(std::current_exception()); }
};

template<>
struct promise_frame_base<void> {
    outcome<void> result;

    void unhandled_exception() noexcept { result.set_exception(std::current_exception()); }
    void return_void() { result.set_value(); }
};

struct promise_no_base_tag {};

template<>
struct promise_frame_base<promise_no_base_tag> {
};

template<class T>
inline std::shared_ptr<void const> get_shared_this(T&& this_)
{
    if constexpr (requires(std::remove_reference_t<T>& t) { { t.shared_from_this() } -> std::convertible_to<std::shared_ptr<void const>>; })
        return this_.shared_from_this();
    else
        return nullptr;
}

template<class... Ts>
struct promise_frame : promise_frame_base<Ts...> {
    unique_handle<>                            caller;
    std::shared_ptr<void const>                shared_this;
    std::unique_ptr<boost::asio::steady_timer> cached_steady_timer;
    boost::asio::any_io_executor               executor;
    boost::asio::any_io_executor const*        caller_executor = std::addressof(executor);
    boost::asio::any_io_executor               inner_executor;
    boost::asio::cancellation_slot             cancellation_slot;
    boost::asio::cancellation_state            cancellation_state;

    promise_frame() = default;

    template<class T, class... Args>
    promise_frame(T&& this_, Args&&...) noexcept
        : shared_this { get_shared_this(std::forward<T>(this_)) }
    {}

    void* operator new(std::size_t size)
    {
        return memory_cache::tls::alloc(size);
    }

    void operator delete(void* ptr, std::size_t size)
    {
        memory_cache::tls::free(ptr, size);
    }

    template<class Executor>
    void set_executor(Executor&& ex)
    {
        executor = std::forward<Executor>(ex);
        inner_executor = nullptr;
    }

    template<class InnerExecutor>
    void set_strand_executor(boost::asio::strand<InnerExecutor> const& ex)
    {
        executor = ex;
        inner_executor = ex.get_inner_executor();
    }

    boost::asio::any_io_executor const& get_executor() const noexcept
    {
        return executor ? executor : *caller_executor;
    }

    boost::asio::cancellation_slot get_cancellation_slot() const noexcept
    {
        auto slot = cancellation_state.slot();
        return slot.is_connected() ? slot : cancellation_slot;
    }

    template<class OtherPromise>
    void on_initial_resume(std::coroutine_handle<OtherPromise> h)
    {
        if constexpr (requires (OtherPromise& p) { { p.get_executor() } -> std::same_as<boost::asio::any_io_executor const&>; })
            caller_executor = std::addressof(h.promise().get_executor());

        if constexpr (requires (OtherPromise& p) { { p.get_cancellation_slot() } -> std::convertible_to<boost::asio::cancellation_slot>; })
            cancellation_slot = h.promise().get_cancellation_slot();

        caller.reset(h);
    }

    std::coroutine_handle<> on_final_resume()
    {
        if (caller_executor != std::addressof(executor) && *caller_executor) {
            boost::asio::dispatch(*caller_executor, detail::resume_handler { std::move(caller) });
            return std::noop_coroutine();
        }
        else if (inner_executor) {
            boost::asio::dispatch(inner_executor, detail::resume_handler { std::move(caller) });
            return std::noop_coroutine();
        }

        return caller.release();
    }

    struct final_awaitable : std::suspend_always {
        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_frame> h) const noexcept
        {
            return h.promise().on_final_resume();
        }
    };

    promise<Ts...> get_return_object() noexcept { return this; }
    std::suspend_always initial_suspend() const noexcept { return {}; }
    final_awaitable final_suspend() const noexcept  { return {}; }

    template<class... Us>
    struct coro_awaitable {
        std::coroutine_handle<promise_frame<Us...>> callee_;

        constexpr bool await_ready() const noexcept { return false; }

        template<class Caller>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Caller> caller) noexcept
        {
            callee_.promise().on_initial_resume(caller);
            return callee_;
        }

        decltype(auto) await_resume()
        {
            unique_handle<promise_frame<Us...>> h { callee_ };
            return h->result.get();
        }
    };

    template<class... Us>
    auto await_transform(promise<Us...> coro) noexcept
    {
        return coro_awaitable<Us...> { coro.m_coro.release() };
    }

    template<class... Args, class Initiation, class... InitArgs>
    auto await_transform(boost::asio::deferred_async_operation<void(Args...), Initiation, InitArgs...> op) const noexcept
    {
        return detail::deferred_awaitable<void(Args...), Initiation, InitArgs...> { std::move(op) };
    }

    auto await_transform(this_coro::post_t arg) noexcept
    {
        struct awaitable : std::suspend_always {
            boost::asio::any_io_executor const& executor_;

            void await_suspend(std::coroutine_handle<> h) const noexcept
            {
                boost::asio::post(executor_, detail::resume_handler { h });
            }
        };

        set_executor(std::move(arg.executor));
        return awaitable { {}, get_executor() };
    }

    auto await_transform(this_coro::dispatch_t arg) noexcept
    {
        struct awaitable : std::suspend_always {
            boost::asio::any_io_executor const& executor_;

            void await_suspend(std::coroutine_handle<> h) const noexcept
            {
                boost::asio::dispatch(executor_, detail::resume_handler { h });
            }
        };

        set_executor(std::move(arg.executor));
        return awaitable { {}, get_executor() };
    }

    auto await_transform(this_coro::executor_t) noexcept
    {
        return detail::getter_awaitable<boost::asio::any_io_executor const&> { get_executor() };
    }

    auto await_transform(this_coro::cancelled_t) noexcept
    {
        return detail::getter_awaitable<boost::asio::cancellation_type> { cancellation_state.cancelled() };
    }

    auto await_transform(this_coro::cancellation_slot_t) noexcept
    {
        return detail::getter_awaitable<boost::asio::cancellation_slot> { get_cancellation_slot() };
    }

    auto await_transform(this_coro::set_cancellation_slot_t arg)
    {
        cancellation_slot = arg.slot;
        cancellation_state = boost::asio::cancellation_state {};
        return std::suspend_never { };
    }

    auto await_transform(this_coro::reset_cancellation_state_t<void>)
    {
        cancellation_state = boost::asio::cancellation_state { cancellation_slot };
        return std::suspend_never { };
    }

    auto await_transform(this_coro::reset_cancellation_state_t<this_coro::set_cancellation_slot_t> arg)
    {
        cancellation_slot = arg.slot;
        cancellation_state = boost::asio::cancellation_state { cancellation_slot };
        return std::suspend_never { };
    }

    template<class Filter>
    auto await_transform(this_coro::reset_cancellation_state_t<void, Filter> arg)
    {
        cancellation_state = boost::asio::cancellation_state { cancellation_slot, std::move(arg.filter) };
        return std::suspend_never { };
    }

    template<class Filter>
    auto await_transform(this_coro::reset_cancellation_state_t<this_coro::set_cancellation_slot_t, Filter> arg)
    {
        cancellation_slot = arg.slot;
        cancellation_state = boost::asio::cancellation_state { cancellation_slot, std::move(arg.filter) };
        return std::suspend_never { };
    }

    template<class InFilter, class OutFilter>
    auto await_transform(this_coro::reset_cancellation_state_t<void, InFilter, OutFilter> arg)
    {
        cancellation_state = boost::asio::cancellation_state { cancellation_slot, std::move(arg.in_filter), std::move(arg.out_filter) };
        return std::suspend_never { };
    }

    template<class InFilter, class OutFilter>
    auto await_transform(this_coro::reset_cancellation_state_t<this_coro::set_cancellation_slot_t, InFilter, OutFilter> arg)
    {
        cancellation_slot = arg.slot;
        cancellation_state = boost::asio::cancellation_state { cancellation_slot, std::move(arg.in_filter), std::move(arg.out_filter) };
        return std::suspend_never { };
    }

    template<class Rep, class Period>
    auto await_transform(this_coro::sleep_t<std::chrono::duration<Rep, Period>> sleep)
    {
        if (!cached_steady_timer)
            cached_steady_timer = std::make_unique<boost::asio::steady_timer>(get_executor());

        return detail::basic_timer_awaitable<boost::asio::steady_timer&> { *cached_steady_timer, sleep.duration };
    }

    template<class Rep, class Period>
    auto await_transform(this_coro::sleep_t<std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<Rep, Period>>> sleep)
    {
        if (!cached_steady_timer)
            cached_steady_timer = std::make_unique<boost::asio::steady_timer>(get_executor());

        return detail::basic_timer_awaitable<boost::asio::steady_timer&> { *cached_steady_timer, sleep.expiry_time };
    }

    template<class Clock, class Rep, class Period>
    auto await_transform(this_coro::sleep_t<std::chrono::time_point<Clock, std::chrono::duration<Rep, Period>>> sleep)
    {
        return detail::timer_awaitable<Clock> { get_executor(), sleep.expiry_time };
    }
};

struct init_await;

} // namespace detail
} // namespace webasio::coro
