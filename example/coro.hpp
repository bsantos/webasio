#pragma once

#include <webasio/coro/detached.hpp>
#include <webasio/coro/promise.hpp>

namespace webasio {

co_promise<int> do_nothing();
co_detached say_hello(boost::asio::any_io_executor const& ex);
co_promise<void> sleep();
co_promise<void> test_sleep();
co_promise<void> wait_for_signal();

} // namespace webasio
