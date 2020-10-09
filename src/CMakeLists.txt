cmake_minimum_required(VERSION 2.4)

project(ostore)

include_directories(../inc ../export)
add_library(ostore SHARED ostore_internal.c ostore.c)


