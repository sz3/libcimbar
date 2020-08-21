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

add_custom_command(
	TARGET cimbar_scan POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar_scan> cimbar_scan.dbg
	COMMAND ${CMAKE_STRIP} -g $<TARGET_FILE:cimbar_scan>
)

install(
	TARGETS cimbar_scan
	DESTINATION bin
)

