//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <webasio/assert.hpp>
#include <webasio/logger.hpp>

#include <cstdlib>

void webasio::bug_trap(const char* msg, std::source_location loc)
{
    webasio::logger { "assert.trap" }.error(loc.file_name(), '(', loc.line(), "): ", loc.function_name(), ": ", msg);
    __builtin_debugtrap();
}

void webasio::bug_abort(const char* msg, std::source_location loc)
{
    webasio::logger { "assert.abort" }.error(loc.file_name(), '(', loc.line(), "): ", loc.function_name(), ": ", msg);
    __builtin_debugtrap();
    std::abort();
}
