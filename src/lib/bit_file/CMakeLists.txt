cmake_minimum_required(VERSION 3.10)

set(SOURCES
	bitreader.h
	bitwriter.h
)

add_custom_target(bit_file SOURCES ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
