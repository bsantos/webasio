#pragma once

#include <coroutine>
#include <memory>
#include <type_traits>

namespace webasio::coro {

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
