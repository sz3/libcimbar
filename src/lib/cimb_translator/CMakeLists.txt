cmake_minimum_required(VERSION 3.10)

set(SOURCES
	CellDrift.h
	CellPosition.h
	CimbCommon.h
	CimbDecoder.cpp
	CimbDecoder.h
	CimbEncoder.cpp
	CimbEncoder.h
	CimbReader.cpp
	CimbReader.h
	CimbWriter.cpp
	CimbWriter.h
	ICimbWriter.h
)

add_library(cimb_translator STATIC ${SOURCES})

add_subdirectory(test)
