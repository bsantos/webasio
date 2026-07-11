//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <atomic>
#include <format>

namespace webasio {
enum class log_level {
	trace,
	debug,
	info,
	notice,
	warn,
	error,
	fatal,
};

class logger final {
public:
	static std::atomic<log_level> level;

private:
	template<size_t N>
	struct format_string {
		inline static constexpr size_t size = N * 2;

		constexpr format_string() noexcept
		{
			for (size_t i = 0; i < size; i += 2)
				std::copy_n("{}", 2, chars + i);
		}

		constexpr std::string_view get() const noexcept { return { chars, size }; }

		char chars[size];
	};

public:
	constexpr explicit logger(std::string_view name)
		: m_name { name }
	{}

	template<class ...Args>
	void trace(Args&& ...args) const
	{
		if (logger::level.load() > log_level::trace)
			return;

		log(log_level::trace, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void debug(Args&& ...args) const
	{
		if (logger::level.load() > log_level::debug)
			return;

		log(log_level::debug, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void info(Args&& ...args) const
	{
		if (logger::level.load() > log_level::info)
			return;

		log(log_level::info, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void notice(Args&& ...args) const
	{
		if (logger::level.load() > log_level::info)
			return;

		notice(log_level::info, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void warn(Args&& ...args) const
	{
		if (logger::level.load() > log_level::warn)
			return;

		log(log_level::warn, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void error(Args&& ...args) const
	{
		if (logger::level.load() > log_level::error)
			return;

		log(log_level::error, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void fatal(Args&& ...args) const
	{
		log(log_level::fatal, std::forward<Args>(args)...);
	}

	template<class ...Args>
	void log(log_level level, Args&& ...args) const
	{
		constexpr format_string<sizeof...(Args)> fmt {};
		do_log(m_name, level, std::vformat(fmt.get(), std::make_format_args(args...)));
	}

private:
	static void do_log(std::string_view name, log_level level, std::string msg);

private:
	std::string_view m_name;
};

} // namespace webasio
