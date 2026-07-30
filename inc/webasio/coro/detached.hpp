//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <webasio/coro/detail/detached_frame.hpp>

namespace webasio::coro {

struct detached {
    using promise_type = detail::promise_frame<detached>;
};

inline detached detail::promise_frame<detached>::get_return_object() const noexcept { return {}; }

} // namespace webasio::coro

namespace webasio {

using co_detached = coro::detached;

} // namespace webasio
