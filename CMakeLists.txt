cmake_minimum_required(VERSION 3.28)
project(webasio)

set(CMAKE_CXX_STANDARD 20)
set(BOOST_INCLUDE_LIBRARIES asio test)

add_compile_options(-Wall -Wextra -Werror -glldb)
add_compile_options(-Wno-unused-parameter)

enable_testing()

add_subdirectory(deps/boost)
add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(tests)
