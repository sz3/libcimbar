cmake_minimum_required(VERSION 3.10)

set(SOURCES
	AdjacentCellFinder.cpp
	AdjacentCellFinder.h
	Cell.h
	CellDrift.cpp
	CellDrift.h
	CellPositions.cpp
	CellPositions.h
	CimbDecoder.cpp
	CimbDecoder.h
	CimbEncoder.cpp
	CimbEncoder.h
	CimbReader.cpp
	CimbReader.h
	CimbWriter.cpp
	CimbWriter.h
	Config.cpp
	Config.h
	FloodDecodePositions.cpp
	FloodDecodePositions.h
	Interleave.h
	LinearDecodePositions.h
	PositionData.h
)

add_library(cimb_translator STATIC ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
