cmake_minimum_required(VERSION 3.28)

project(webasio LANGUAGES CXX)

add_library(webasio
  ../inc/webasio/asio/cancellation_proxy.hpp
  ../inc/webasio/asio/yield.hpp
  ../inc/webasio/assert.hpp
  ../inc/webasio/cast.hpp
  ../inc/webasio/coro/await.hpp
  ../inc/webasio/coro/cancellation.hpp
  ../inc/webasio/coro/defer.hpp
  ../inc/webasio/coro/detached.hpp
  ../inc/webasio/coro/detail/await.hpp
  ../inc/webasio/coro/detail/basic_detached.hpp
  ../inc/webasio/coro/detail/completion_handler.hpp
  ../inc/webasio/coro/detail/deferred_awaitable.hpp
  ../inc/webasio/coro/detail/detached_frame.hpp
  ../inc/webasio/coro/detail/dispatch_handler.hpp
  ../inc/webasio/coro/detail/getter_awaitable.hpp
  ../inc/webasio/coro/detail/promise_frame.hpp
  ../inc/webasio/coro/detail/resume_context.hpp
  ../inc/webasio/coro/detail/timer_awaitable.hpp
  ../inc/webasio/coro/dispatch.hpp
  ../inc/webasio/coro/executor.hpp
  ../inc/webasio/coro/main.hpp
  ../inc/webasio/coro/outcome.hpp
  ../inc/webasio/coro/post.hpp
  ../inc/webasio/coro/promise.hpp
  ../inc/webasio/coro/sleep.hpp
  ../inc/webasio/coro/unique_handle.hpp
  ../inc/webasio/logger.hpp
  ../inc/webasio/memory_cache.hpp
  ../inc/webasio/net/stream.hpp
  ../inc/webasio/scoped_final.hpp
  ../inc/webasio/stddef.hpp

  assert.cpp
  logger.cpp
  memory_cache.cpp
)

target_include_directories(webasio PUBLIC ../inc)

target_link_libraries(webasio
  PUBLIC
    Boost::asio
)
