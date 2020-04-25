cmake_minimum_required(VERSION 3.10)

set(SOURCES
	average_hash.h
	hamming_distance.h
)

add_custom_target(image_hash SOURCES ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
