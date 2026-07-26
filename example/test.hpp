#pragma once

#include <webasio/asio/cancellation_proxy.hpp>
#include <webasio/asio/yield.hpp>
#include <webasio/logger.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/prepend.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

#include <iostream>


namespace webasio {

inline constexpr logger test_log { "test" };

struct test_operation {
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    asio::cancellation_proxy proxy_cancel;
    std::unique_ptr<boost::asio::steady_timer> wait;
    size_t count = 3;
    enum class state { start, enter, wait, done } state_ = state::start;

    test_operation(boost::asio::strand<boost::asio::io_context::executor_type> ex, asio::cancellation_proxy&& proxy)
        : strand { std::move(ex) }
        , proxy_cancel { std::move(proxy) }
        , wait { std::make_unique<boost::asio::steady_timer>(strand.get_inner_executor()) }
    {}

    template<class Self>
    void operator()(Self& self, boost::system::error_code ec = {})
    {
        switch (state_) {
        case state::start:
            test_log.info("Start: ", count, ", is-in-strand=", strand.running_in_this_thread());

            // enter the strand executor
            WEBASIO_YIELD(state_, state::enter, boost::asio::dispatch(strand, std::move(self)));

            test_log.info("Enter Loop: ", count, ", is-in-strand=", strand.running_in_this_thread());

            while (!ec && count > 0) {
                wait->expires_after(std::chrono::seconds(1));
                WEBASIO_YIELD(state_, state::wait, wait->async_wait(boost::asio::bind_executor(strand, std::move(self))));
                --count;

                test_log.info("Resume Loop: ", count, ", is-in-strand=", strand.running_in_this_thread(), ", error=", ec.message());
            }

            test_log.info("Exit Loop: ", count, ", is-in-strand=", strand.running_in_this_thread(), ", error=", ec.message());

            // exit the strand executor
            WEBASIO_YIELD(state_, state::done, boost::asio::post(boost::asio::prepend(std::move(self), ec)));

            test_log.info("Complete: ", count, ", is-in-strand=", strand.running_in_this_thread(), ", error=", ec.message());

            proxy_cancel.reset();
            self.complete(ec);
        }
    }
};

template<class CompletionToken = boost::asio::deferred_t>
inline decltype(auto) test(boost::asio::io_context& ioc, CompletionToken&& token = boost::asio::deferred_t {})
{
    boost::asio::strand<boost::asio::io_context::executor_type> strand { ioc.get_executor() };
    asio::cancellation_proxy proxy { strand, token };
    auto ptoken = proxy.bind(token);

    return boost::asio::async_compose<decltype(ptoken), void(boost::system::error_code)>(
        test_operation { std::move(strand), std::move(proxy) },
        ptoken,
        ioc);
}

} // namespace webasio
