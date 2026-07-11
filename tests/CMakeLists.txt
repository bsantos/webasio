cmake_minimum_required(VERSION 3.28)

project(webasio_unit_tests LANGUAGES CXX)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/*.cpp")
add_executable(unit_tests ${SRC_FILES})

target_link_libraries(unit_tests
  PUBLIC
    Boost::unit_test_framework
    webasio
)

