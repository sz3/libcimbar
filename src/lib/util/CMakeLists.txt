cmake_minimum_required(VERSION 3.10)

set(SOURCES
	File.h
	MakeTempDirectory.h
	Timer.h
)

add_library(util INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

