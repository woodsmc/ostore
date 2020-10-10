cmake_minimum_required(VERSION 2.4)

project(ostore VERSION 1.0.0 DESCRIPTION "object storage in single file")



include_directories(../inc ../export)
add_library(ostore SHARED ostore_internal.c ostore.c)

set_target_properties(ostore PROPERTIES SOVERSION 1)
set_target_properties(ostore PROPERTIES PUBLIC_HEADER export/ostore.h)



