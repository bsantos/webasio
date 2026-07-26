//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <webasio/coro/detail/promise_frame.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/container/small_vector.hpp>

#include <coroutine>
#include <span>
#include <string_view>
#include <thread>
#include <vector>


namespace webasio::coro {
namespace detail {
extern "C" int main(int argc, const char* argv[]) noexcept;
} // namespace detail

class main {
    struct promise_frame : detail::promise_frame<detail::promise_no_base_tag> {
        using executor_type = boost::asio::io_context::executor_type;
        using executor_work_guard = boost::asio::executor_work_guard<executor_type>;

        main get_return_object() noexcept { return *this; }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        std::suspend_always final_suspend() const noexcept { return {}; }
        void unhandled_exception() { throw; }

        void return_value(int code) noexcept
        {
            code_ = code;
            work_->get_executor().context().stop();
            work_ = std::nullopt;
        }

        void return_value(std::pair<int, std::chrono::steady_clock::duration> code_and_timeout) noexcept
        {
            code_ = code_and_timeout.first;
            timeout_ = code_and_timeout.second;
            work_->get_executor().context().stop();
            work_ = std::nullopt;
        }

        int code_ = 0;
        std::chrono::steady_clock::duration timeout_ = std::chrono::steady_clock::duration::min();
        std::optional<executor_work_guard> work_;
    };

    struct io_run_loop {
        boost::asio::io_context& ioc;

        void operator()() noexcept
        {
            ioc.run();
        }
    };

    friend int detail::main(int argc, const char* argv[]) noexcept;

    main(promise_frame& p) noexcept
        : handle_ { std::coroutine_handle<promise_frame>::from_promise(p) }
    {}

    main() = delete;
    main(main&&) = delete;
    main(main const&) = delete;
    main& operator=(main&&) = delete;
    main& operator=(main const&) = delete;

    void resume() { handle_.resume(); }

    int code() const noexcept { return handle_.promise().code_; }
    std::chrono::steady_clock::duration timeout() const noexcept { return handle_.promise().timeout_; }

    std::coroutine_handle<promise_frame> handle_;

public:
    using promise_type = promise_frame;

    ~main() { handle_.destroy(); }
};

} // namespace webasio::coro

namespace webasio {
coro::main co_main(std::span<std::string_view> args);
} // namespace webasio

int webasio::coro::detail::main(int argc, const char* argv[]) noexcept
{
    boost::container::small_vector<std::string_view, 5> args { argv, argv + argc };
    auto const concurrency = std::thread::hardware_concurrency();
    boost::asio::io_context ioc { static_cast<int>(concurrency) };
    auto coro = ::webasio::co_main(args);

    coro.handle_.promise().executor = ioc.get_executor();
    coro.handle_.promise().work_.emplace(ioc.get_executor());
    coro.resume();

    if (!ioc.stopped()) {
        std::vector<std::thread> threads;

        threads.reserve(concurrency - 1);

        for (unsigned i = 1; i < concurrency; ++i)
            threads.emplace_back(coro::main::io_run_loop { ioc });

        ioc.run();

        for (auto&& thread : threads)
            thread.join();

        ioc.restart();
        ioc.run_for(coro.timeout());
    }

    return coro.code();
}
