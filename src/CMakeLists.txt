cmake_minimum_required(VERSION 3.28)

project(webasio LANGUAGES CXX)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/*.cpp")
add_library(webasio ${SRC_FILES})

target_include_directories(webasio PUBLIC ../inc)

target_link_libraries(webasio
  PUBLIC
    Boost::asio
)
