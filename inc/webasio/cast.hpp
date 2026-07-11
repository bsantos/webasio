//
// Copyright (c) Bruno Santos (bsantos at cppdev dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <utility>
#include <type_traits>

#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wconversion"
#endif

#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wconversion"
#endif

namespace webasio {

template<class T, class U>
[[nodiscard]]
constexpr T narrow_cast(U&& u) noexcept
{
    return static_cast<T>(std::forward<U>(u));
}

template<class E>
requires std::is_enum_v<E>
[[nodiscard]]
constexpr auto to_underlying_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

} // namespace webasio

#if defined(__clang__)
#	pragma clang diagnostic pop
#endif

#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic pop
#endif
