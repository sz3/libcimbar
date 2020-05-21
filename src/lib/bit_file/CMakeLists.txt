cmake_minimum_required(VERSION 3.10)

set(SOURCES
	bitreader.h
	bitwriter.h
)

add_library(bit_file INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
