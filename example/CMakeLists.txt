cmake_minimum_required(VERSION 3.28)

project(webasio_example LANGUAGES CXX)

add_executable(example
  coro.hpp
  test.hpp

  coro.cpp
  main.cpp
)

target_link_libraries(example
  PUBLIC
    webasio
)
