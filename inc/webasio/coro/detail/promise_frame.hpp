//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/assert.hpp>
#include <webasio/coro/detail/deferred_awaitable.hpp>
#include <webasio/coro/detail/dispatch_handler.hpp>
#include <webasio/memory_cache.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/cancellation_state.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>


namespace webasio::coro {

template<class... Ts>
class promise;

namespace detail {

/**
 * @brief Customization point mapping an awaited value to an awaitable.
 * @internal
 * @details Specialized by feature headers (sleep, dispatch, post, executor,
 * cancellation) to give meaning to `co_await <tag>` inside a coroutine. Each
 * specialization provides a static `get(frame, value)` returning the concrete
 * awaitable, invoked from @ref promise_frame::await_transform.
 */
template<class T>
struct promise_awaitable;

/**
 * @brief Result-carrying base of @ref promise_frame for value coroutines.
 * @internal
 * @details Provides `return_value`/`unhandled_exception` that store the
 * produced value or captured exception into an @ref outcome.
 */
template<class... Ts>
struct promise_frame_base {
    using value_type = typename outcome<Ts...>::value_type;

    outcome<Ts...> result;

    void return_value(value_type&& v) { result.set_value(std::move(v)); }
    void return_value(value_type const& v) { result.set_value(v); }
    void unhandled_exception() noexcept { result.set_exception(std::current_exception()); }
};

/// @internal Base specialization for `void`-returning coroutines.
template<>
struct promise_frame_base<void> {
    outcome<void> result;

    void unhandled_exception() noexcept { result.set_exception(std::current_exception()); }
    void return_void() { result.set_value(); }
};

/// @internal Tag selecting a promise base that carries no result at all.
struct promise_no_base_tag {};

template<>
struct promise_frame_base<promise_no_base_tag> {
};

/// @internal Returns `shared_from_this()` if @p this_ supports it, else null.
template<class T>
inline std::shared_ptr<void const> get_shared_this(T&& this_)
{
    if constexpr (requires(std::remove_reference_t<T>& t) { { t.shared_from_this() } -> std::convertible_to<std::shared_ptr<void const>>; })
        return this_.shared_from_this();
    else
        return nullptr;
}

/**
 * @brief The coroutine promise driving @ref webasio::coro::promise.
 * @internal
 * @details
 * Holds the coroutine's full runtime state: the pending result, the caller
 * handle to resume on completion, an optional strong self-reference
 * (`shared_this`) to keep an owning object alive across suspension, the
 * cancellation slot/state, and the associated executor chain used for
 * executor hopping (`executor`, `inner_executor`, `caller_executor`).
 *
 * Frames are allocated from the thread-local @ref webasio::memory_cache.
 * `await_transform` routes awaited values through @ref promise_awaitable,
 * handles nested `promise` awaiting (propagating executor and cancellation
 * slot to the callee), and adapts raw Asio deferred operations. On final
 * suspension the caller is resumed on its own executor, hopping threads when
 * necessary.
 *
 * @tparam Ts The coroutine's produced value types (or a base tag).
 */
template<class... Ts>
struct promise_frame : promise_frame_base<Ts...> {
    unique_handle<>                            caller;
    std::shared_ptr<void const>                shared_this;
    std::unique_ptr<boost::asio::steady_timer> steady_timer;
    boost::asio::cancellation_slot             cancellation_slot;
    boost::asio::cancellation_state            cancellation_state;
    boost::asio::any_io_executor const*        caller_executor = std::addressof(executor);
    boost::asio::any_io_executor               executor;
    boost::asio::any_io_executor               inner_executor;

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

        if (caller_executor == std::addressof(inner_executor)) {
            caller_executor = std::addressof(executor);
            inner_executor = nullptr;
        }
    }

    template<class InnerExecutor>
    void set_executor(boost::asio::strand<InnerExecutor>&& ex)
    {
        if (caller_executor == std::addressof(executor)) {
            caller_executor = std::addressof(inner_executor);
            inner_executor = ex.get_inner_executor();
        }

        executor = std::move(ex);
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

    /// @internal Resumes the caller on final suspension, hopping to its
    /// executor when it differs from the callee's.
    struct final_awaitable : std::suspend_always, resume_context<final_awaitable> {
        using context = resume_context<final_awaitable>;

        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_frame> callee) noexcept
        {
            promise_frame& callee_frame = callee.promise();
            std::coroutine_handle<> caller = callee_frame.caller.release();

            // ensure the caller coroutine is resumed on it's executor
            if (callee_frame.caller_executor != std::addressof(callee_frame.executor) && *callee_frame.caller_executor) {
                boost::asio::dispatch(*callee_frame.caller_executor, detail::dispatch_handler<final_awaitable> { *this, caller });

                if (this->resume_awaiting(caller))
                    return caller;

                return std::noop_coroutine();
            }

            return caller;
        }
    };

    promise<Ts...> get_return_object() noexcept { return this; }
    std::suspend_always initial_suspend() const noexcept { return {}; }
    final_awaitable final_suspend() const noexcept  { return {}; }

    /// @internal Awaitable for a nested `co_await promise<Us...>`, wiring the
    /// callee's caller/executor/cancellation-slot to this frame.
    template<class... Us>
    struct coro_awaitable {
        std::coroutine_handle<promise_frame<Us...>> callee_;

        constexpr bool await_ready() const noexcept { return false; }

        template<class Caller>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<Caller> caller) noexcept
        {
            callee_.promise().caller.reset(caller);

            if constexpr (requires (Caller& p) { { p.get_executor() } -> std::same_as<boost::asio::any_io_executor const&>; })
                callee_.promise().caller_executor = std::addressof(caller.promise().get_executor());

            if constexpr (requires (Caller& p) { { p.get_cancellation_slot() } -> std::convertible_to<boost::asio::cancellation_slot>; })
                callee_.promise().cancellation_slot = caller.promise().get_cancellation_slot();

            return callee_;
        }

        decltype(auto) await_resume()
        {
            unique_handle<promise_frame<Us...>> h { callee_ };
            return h->result.get();
        }
    };

    /// @internal Transforms `co_await promise<Us...>` into @ref coro_awaitable.
    template<class... Us>
    auto await_transform(promise<Us...> coro) noexcept
    {
        return coro_awaitable<Us...> { coro.m_coro.release() };
    }

    /// @internal Transforms a raw Asio deferred operation into a
    /// @ref deferred_awaitable so it can be awaited directly.
    template<class... Args, class Initiation, class... InitArgs>
    auto await_transform(boost::asio::deferred_async_operation<void(Args...), Initiation, InitArgs...> op) noexcept
    {
        return detail::deferred_awaitable<promise_frame, void(Args...), Initiation, InitArgs...> { *this, std::move(op) };
    }

    /// @internal Transforms any other awaited value via @ref promise_awaitable.
    template<class T>
    auto await_transform(T&& value) -> decltype(promise_awaitable<std::decay_t<T>>::get(*this, std::forward<T>(value)))
    {
        return promise_awaitable<std::decay_t<T>>::get(*this, std::forward<T>(value));
    }
};

struct init_await;

} // namespace detail
} // namespace webasio::coro
