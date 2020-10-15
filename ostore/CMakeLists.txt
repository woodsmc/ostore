cmake_minimum_required(VERSION 3.16)

project(ostore VERSION 1.0.0 DESCRIPTION "object storage in single file")
option(FULL_LIB_BUILD "Build as library" ON)
option(CMAKE_BUILD_ON "Mark as being build with CMake, and all the version info, etc." ON)

configure_file(../inc/ostoreconfig.h.in ostoreconfig.h)


include_directories(../inc ../export)
add_library(ostore SHARED ostore_internal.c ostore.c)

# set_target_properties(ostore PROPERTIES SOVERSION 1)
# set_target_properties(ostore PROPERTIES PUBLIC_HEADER export/ostore.h)

target_include_directories(ostore PUBLIC "${PROJECT_BINARY_DIR}")



install(TARGETS ostore DESTINATION lib)
install(FILES ../export/ostore.h DESTINATION include)