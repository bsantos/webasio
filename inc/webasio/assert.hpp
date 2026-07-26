//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <source_location>

namespace webasio {

void bug_trap(const char* msg, std::source_location loc = std::source_location::current());

[[noreturn]]
void bug_abort(const char* msg, std::source_location loc = std::source_location::current());

inline void assert_trap(bool cond, const char* msg, std::source_location loc = std::source_location::current())
{
    if (cond) [[unlikely]]
        return;

    bug_trap(msg, loc);
}

inline void assert_abort(bool cond, const char* msg, std::source_location loc = std::source_location::current())
{
    if (cond) [[unlikely]]
        return;

    bug_abort(msg, loc);
}

} // namespace webasio
