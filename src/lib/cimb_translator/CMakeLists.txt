cmake_minimum_required(VERSION 3.10)

set(SOURCES
	CimbEncoder.cpp
	CimbEncoder.h
)

add_library(cimb_translator STATIC ${SOURCES})

add_subdirectory(test)
