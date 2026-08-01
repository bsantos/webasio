cmake_minimum_required(VERSION 3.28)

project(webasio_unit_tests LANGUAGES CXX)

add_executable(unit_tests EXCLUDE_FROM_ALL
  coro/await.cpp
  coro/basic_detached.cpp
  coro/cancellation.cpp
  coro/defer.cpp
  coro/detached.cpp
  coro/executor.cpp
  coro/promise.cpp
  main.cpp
)

target_link_libraries(unit_tests
  PUBLIC
    Boost::unit_test_framework
    webasio
)

add_test(NAME unit_tests COMMAND unit_tests)

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS unit_tests
)

