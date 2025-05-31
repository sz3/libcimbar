cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Anchor.h
	Corners.h
	DistortionParameters.h
	EdgeScanState.h
	Geometry.h
	Midpoints.h
	Point.h
	ScanState.h
	Scanner.cpp
	Scanner.h
	SimpleCameraCalibration.cpp
	SimpleCameraCalibration.h
	SimpleDeskewer.cpp
	SimpleDeskewer.h
	SimpleExtractor.cpp
	SimpleExtractor.h
	Undistort.h
)

add_library(extractor STATIC ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
