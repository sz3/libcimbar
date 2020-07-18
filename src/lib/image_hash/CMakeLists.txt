cmake_minimum_required(VERSION 3.10)

set(SOURCES
	ahash_result.h
	average_hash.h
	bit_extractor.h
	hamming_distance.h
)

add_library(image_hash INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
