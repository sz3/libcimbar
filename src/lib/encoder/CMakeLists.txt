cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Decoder.h
	Encoder.h
	ReedSolomon.h
	SimpleEncoder.h
	reed_solomon_stream.h
)

add_library(encoder INTERFACE)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

