cmake_minimum_required(VERSION 3.10)

project(cimbar_scan)

set (SOURCES
	cimbar_scan.cpp
)

add_executable (
	cimbar_scan
	${SOURCES}
)

target_link_libraries(cimbar_scan

	extractor

	${OPENCV_LIBS}
)


