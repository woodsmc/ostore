cmake_minimum_required(VERSION 3.16)

project(ostorecmd VERSION 1.0)

configure_file(ostorecmdconfig.h.in ostorecmdconfig.h)

add_executable(ostorecmd parameters.cpp commandlinetool.cpp)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(ostorecmd PUBLIC ostore)

target_include_directories(ostorecmd PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(ostorecmd PUBLIC "../export")

