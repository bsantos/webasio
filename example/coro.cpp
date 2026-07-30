//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "coro.hpp"

#include <webasio/logger.hpp>
#include <webasio/coro/dispatch.hpp>
#include <webasio/coro/executor.hpp>
#include <webasio/coro/sleep.hpp>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>


namespace webasio {

inline constexpr logger example_log { "example" };

co_promise<int> do_nothing()
{
    co_return 42;
}

co_detached say_hello(boost::asio::any_io_executor const& ex)
{
    example_log.info("hello");
    co_await coro::dispatch(ex);
    co_await sleep();
    co_return;
}

co_promise<void> sleep()
{
	example_log.info("sleeping");
	co_await coro::sleep_for(std::chrono::seconds(1));
	example_log.info("sleeping again");
    co_await coro::sleep_for(std::chrono::seconds(1));
    example_log.info("sleeping completed");
}

co_detached do_sleep(boost::asio::any_io_executor ex, std::chrono::seconds d)
{
	boost::asio::steady_timer timer { ex, d };

	example_log.info("sleeping for ", d);
	co_await timer.async_wait();
	example_log.info("wakeup after ", d);
}

co_promise<void> test_sleep()
{
	for (size_t i = 0; i < 1000; ++i)
		do_sleep(co_await coro::executor, std::chrono::seconds(i & 0x3));
}

co_promise<void> wait_for_signal()
{
	boost::asio::signal_set sig { co_await coro::executor, SIGINT, SIGTERM };
	example_log.info("waiting for signal");
	auto [ec, sig_num] = co_await sig.async_wait(boost::asio::as_tuple);
	if (sig_num == SIGTERM)
		example_log.info("exiting because of terminate signal");
	else if (sig_num == SIGINT)
		example_log.info("\nexiting because of terminal interrupt");
	else
		example_log.info("exit by cancelation");
}

} // namespace webasio
