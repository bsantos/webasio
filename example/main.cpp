//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "coro.hpp"

#include <webasio/coro/main.hpp>
#include <webasio/logger.hpp>


namespace webasio {

inline constexpr logger main_log { "main" };

coro::main co_main(std::span<std::string_view> args)
{
	main_log.info("do_nothing(): ", co_await do_nothing());
	say_hello(co_await this_coro::executor);
	co_await sleep();
	co_await wait_for_signal();
	main_log.info("done! will wait at most 5s for pending operations");

	co_return { 0, std::chrono::seconds { 5 } };
}

} // namespace webasio
