cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Decoder.cpp
	Decoder.h
	Encoder.h
	ReedSolomon.h
	reed_solomon_stream.h
)

add_library(encoder STATIC ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

