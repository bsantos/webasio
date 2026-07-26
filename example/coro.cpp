//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "coro.hpp"

#include <webasio/logger.hpp>

#include <boost/asio/signal_set.hpp>

namespace webasio {

inline constexpr logger example_log { "example" };

co_promise<int> do_nothing()
{
    co_return 42;
}

co_detached say_hello(boost::asio::any_io_executor const& ex)
{
    example_log.info("hello");
    co_await this_coro::dispatch(ex);
    co_await sleep();
    co_return;
}

co_promise<void> sleep()
{
	example_log.info("sleeping");
	co_await this_coro::sleep_for(std::chrono::seconds(1));
	example_log.info("sleeping again");
    co_await this_coro::sleep_for(std::chrono::seconds(1));
    example_log.info("sleeping completed");
}

co_promise<void> wait_for_signal()
{
	boost::asio::signal_set sig { co_await this_coro::executor, SIGINT, SIGTERM };
	example_log.info("waiting for signal");
	auto [ec, sig_num] = co_await sig.async_wait();
	if (sig_num == SIGTERM)
		example_log.info("exiting because of terminate signal");
	else if (sig_num == SIGINT)
		example_log.info("\nexiting because of terminal interrupt");
	else
		example_log.info("exit by cancelation");
}

} // namespace webasio
