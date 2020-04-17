cmake_minimum_required(VERSION 3.10)

set(SOURCES
	average_hash.h
	hamming_distance.h
)

add_custom_target(image_hash SOURCES ${SOURCES})

add_subdirectory(test)
