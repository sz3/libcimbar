cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Decoder.cpp
	Decoder.h
	Encoder.cpp
	Encoder.h
	ReedSolomon.h
	ReedSolomonFile.h
)

add_library(encoder STATIC ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()

