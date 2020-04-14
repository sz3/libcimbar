cmake_minimum_required(VERSION 3.10)

set(SOURCES
	CellPosition.h
	CimbEncoder.cpp
	CimbEncoder.h
	CimbWriter.cpp
	CimbWriter.h
	ICimbWriter.h
)

add_library(cimb_translator STATIC ${SOURCES})

add_subdirectory(test)
