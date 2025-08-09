cmake_minimum_required(VERSION 3.10)

set(SOURCES
	zstd_compressor.h
	zstd_decompressor.h
	zstd_header_check.h
)

add_library(compression INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

