//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <webasio/logger.hpp>

#include <webasio/cast.hpp>

#include <iostream>
#include <format>

#if defined(__linux__)
#	include <sys/syscall.h>
#	include <syslog.h>
#	include <unistd.h>
#elif defined(__APPLE__)
#	include <pthread.h>
#	include <syslog.h>
#endif

namespace webasio {
namespace {
constexpr std::array<std::string_view, 7> k_log_level_names {
	"TRACE",
	"DEBUG",
	"INFO",
	"NOTICE"
	"WARN",
	"ERROR",
	"FATAL",
};

thread_local int64_t tls_thread_id;

int64_t thread_id()
{
	if (!tls_thread_id) {
#if defined(__linux__)
		tls_thread_id = syscall(__NR_gettid);

#elif defined(__APPLE__)
		uint64_t id;

		pthread_threadid_np(nullptr, &id);
		tls_thread_id = narrow_cast<int64_t>(id);
#else
#error Not supported
#endif
	}

	return tls_thread_id;
}

void system_log(log_level lvl, std::string_view entry)
{
#if defined(__linux__) || defined(__APPLE__)
	int priority;

	switch (lvl) {
		case log_level::trace: [[fallthrough]];
		case log_level::debug: priority = LOG_DEBUG; break;
		case log_level::info:  priority = LOG_INFO; break;
		case log_level::notice: priority = LOG_NOTICE; break;
		case log_level::warn:  priority = LOG_WARNING; break;
		case log_level::error: priority = LOG_ERR; break;
		case log_level::fatal: priority = LOG_CRIT; break;
	}

	::syslog(priority, "%.*s", narrow_cast<int>(entry.size()), entry.data());
#endif
}

std::string to_string(std::chrono::system_clock::time_point tp)
{
	auto time = std::chrono::system_clock::to_time_t(tp);
	auto tm = std::localtime(&time);
	std::string str;

	str.resize(128);
	auto sz = std::strftime(str.data(), str.size() + 1, "%c", tm);
	while (!sz) [[unlikely]] {
		str.resize(str.size() * 2);
		sz = std::strftime(str.data(), str.size() + 1, "%c", tm);
	}

	str.resize(sz);
	return str;
}

} // namspace

std::atomic<log_level> logger::level { log_level::info };

void logger::do_log(std::string_view name, log_level lvl, std::string msg)
{
	auto now = std::chrono::system_clock::now();

	system_log(level, msg);

	std::string entry = std::format("{} [{}] |{}|{}| {}\n", to_string(now), thread_id(), k_log_level_names[to_underlying_type(lvl)], name, msg);

	if (lvl < log_level::error)
		std::cout << entry << std::flush;
	else
		std::cerr << entry << std::flush;

	if (lvl == log_level::fatal)
		std::abort();
}

} // namespace webasio
