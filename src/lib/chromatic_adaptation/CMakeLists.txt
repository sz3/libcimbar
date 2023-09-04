cmake_minimum_required(VERSION 3.10)

set(SOURCES
	adaptation_transform.h
	color_correction.h
	nlargest.h
)

add_library(chromatic_adaptation INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
