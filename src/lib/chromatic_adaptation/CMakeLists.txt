cmake_minimum_required(VERSION 3.10)

set(SOURCES
	color_correction.h
	von_kries.h
)

add_library(chromatic_adaptation INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
