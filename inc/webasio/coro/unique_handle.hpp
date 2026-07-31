#pragma once

#include <coroutine>
#include <memory>
#include <type_traits>

namespace webasio::coro {

/**
 * @brief Unique-ownership RAII wrapper around a `std::coroutine_handle`.
 *
 * @details
 * Behaves like `std::unique_ptr` for coroutine frames: it owns a single
 * `std::coroutine_handle<T>` and destroys the frame on scope exit unless
 * ownership is released or reset. Copy operations are deleted; move
 * operations transfer ownership. When @p T is the promise type, the handle
 * can be constructed from a promise reference and dereferenced to access it.
 *
 * @tparam T The coroutine promise type, or `void` for a type-erased handle.
 */
template<class T = void>
class unique_handle {
public:
    constexpr unique_handle() noexcept
    {}

    constexpr unique_handle(std::nullptr_t) noexcept
    {}

    explicit unique_handle(T* p) noexcept
        : m_handle { std::coroutine_handle<T>::from_promise(*p) }
    {}

    explicit unique_handle(std::coroutine_handle<T> h) noexcept
        : m_handle { h }
    {}

    unique_handle(unique_handle&& other) noexcept
        : m_handle { other.release() }
    {}

    ~unique_handle()
    {
        if (m_handle)
            m_handle.destroy();
    }

    unique_handle& operator=(unique_handle&& other) noexcept
    {
        m_handle = other.release();
        return *this;
    }

    T* get() const noexcept { return &m_handle.promise(); }
    T& operator*() const noexcept { return m_handle.promise(); }
    T* operator->() const noexcept { return &m_handle.promise(); }

    void reset(std::nullptr_t = nullptr)
    {
        if (m_handle)
            release().destroy();
    }

    void reset(std::coroutine_handle<T> h)
    {
        reset();
        m_handle = h;
    }

    void reset(T* p)
    {
        reset(std::coroutine_handle<T>::from_promise(*p));
    }

    [[nodiscard]] bool done() const noexcept { return m_handle.done(); }
    [[nodiscard]] void* address() const noexcept { return m_handle.address(); }
    [[nodiscard]] std::coroutine_handle<T> release() noexcept { return std::exchange(m_handle, nullptr); }

    void operator()() { release().resume(); }
    void resume() { release().resume(); }

    explicit operator bool() const noexcept { return static_cast<bool>(m_handle); }

    friend bool operator==(unique_handle const&, unique_handle const&) = default;
    friend auto operator<=>(unique_handle const&, unique_handle const&) = default;

private:
    unique_handle(unique_handle const&) noexcept = delete;
    unique_handle& operator=(unique_handle const&) noexcept = delete;

    std::coroutine_handle<T> m_handle;
};

/**
 * @brief Type-erased specialization of @ref unique_handle.
 * @details Owns a `std::coroutine_handle<>` without knowledge of the promise
 * type, so it exposes lifetime/resume operations but no promise access.
 */
template<>
class unique_handle<void> {
public:
    constexpr unique_handle() noexcept
    {}

    constexpr unique_handle(std::nullptr_t) noexcept
    {}

    explicit unique_handle(std::coroutine_handle<> h) noexcept
        : m_handle { h }
    {}

    unique_handle(unique_handle&& other) noexcept
        : m_handle { other.release() }
    {}

    ~unique_handle()
    {
        if (m_handle)
            m_handle.destroy();
    }

    unique_handle& operator=(unique_handle&& other) noexcept
    {
        m_handle = other.release();
        return *this;
    }

    void reset(std::nullptr_t = nullptr)
    {
        if (m_handle)
            release().destroy();
    }

    void reset(std::coroutine_handle<> h)
    {
        reset();
        m_handle = h;
    }

    [[nodiscard]] bool done() const noexcept { return m_handle.done(); }
    [[nodiscard]] void* address() const noexcept { return m_handle.address(); }
    [[nodiscard]] std::coroutine_handle<> release() noexcept { return std::exchange(m_handle, nullptr); }

    void operator()() { release().resume(); }
    void resume() { release().resume(); }

    explicit operator bool() const noexcept { return static_cast<bool>(m_handle); }

    friend bool operator==(unique_handle const&, unique_handle const&) = default;
    friend auto operator<=>(unique_handle const&, unique_handle const&) = default;

private:
    unique_handle(unique_handle const&) noexcept = delete;
    unique_handle& operator=(unique_handle const&) noexcept = delete;

    std::coroutine_handle<> m_handle;
};

} // namespace webasio::coro
