cmake_minimum_required(VERSION 3.16)

project(examplenewobject VERSION 1.0.0 DESCRIPTION "Creates a new ostore file and stores an example string")

# set_target_properties(ostore PROPERTIES SOVERSION 1)
# set_target_properties(ostore PROPERTIES PUBLIC_HEADER export/ostore.h)

add_executable(examplenewobject writeanewobject.c)

# include_directories("${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(examplenewobject PUBLIC ostore)


