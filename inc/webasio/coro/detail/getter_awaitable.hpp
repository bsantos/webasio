#pragma once

namespace webasio::coro::detail {

template<class T, class U = T>
struct getter_awaitable {
    U value;

    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(std::coroutine_handle<> h) const noexcept {}
    constexpr T await_resume() const noexcept(noexcept(T(std::declval<U>()))) { return value; }
};

} // namespace detail
