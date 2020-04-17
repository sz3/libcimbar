cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Anchor.h
	Corners.h
	Deskewer.cpp
	Deskewer.h
	ScanState.cpp
	ScanState.h
	Scanner.cpp
	Scanner.h
)

add_library(extractor STATIC ${SOURCES})

add_subdirectory(test)
