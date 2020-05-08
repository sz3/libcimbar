cmake_minimum_required(VERSION 3.10)

set(SOURCES
	FountainDecoder.h
	FountainEncoder.h
	WirehairInit.h
)

add_custom_target(fountain SOURCES ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

