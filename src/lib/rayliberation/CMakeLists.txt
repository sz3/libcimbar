cmake_minimum_required(VERSION 3.10)

add_library(rayliberation INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
