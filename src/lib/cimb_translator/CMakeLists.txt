cmake_minimum_required(VERSION 3.10)

set(SOURCES
	bitmaps.h
	CellDrift.cpp
	CellDrift.h
	CellPosition.cpp
	CellPosition.h
	CimbDecoder.cpp
	CimbDecoder.h
	CimbEncoder.cpp
	CimbEncoder.h
	CimbReader.cpp
	CimbReader.h
	CimbWriter.cpp
	CimbWriter.h
	Common.cpp
	Common.h
	Config.cpp
	Config.h
	ICimbWriter.h
)

add_library(cimb_translator STATIC ${SOURCES})

add_subdirectory(test)
