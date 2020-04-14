cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Decoder.cpp
	Decoder.h
	Encoder.cpp
	Encoder.h
)

add_library(encoder STATIC ${SOURCES})

add_subdirectory(test)
