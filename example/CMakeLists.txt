cmake_minimum_required(VERSION 3.28)

project(webasio_example LANGUAGES CXX)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/*.cpp")
add_executable(example ${SRC_FILES})

target_link_libraries(example
  PUBLIC
    webasio
)
