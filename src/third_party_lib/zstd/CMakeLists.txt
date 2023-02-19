cmake_minimum_required(VERSION 3.10)
project(zstd C)

add_subdirectory(common)
add_subdirectory(compress)
add_subdirectory(decompress)

set(zstd_obj_files $<TARGET_OBJECTS:zstd-common> $<TARGET_OBJECTS:zstd-compress> $<TARGET_OBJECTS:zstd-decompress>)

add_library(zstd ${zstd_obj_files})

