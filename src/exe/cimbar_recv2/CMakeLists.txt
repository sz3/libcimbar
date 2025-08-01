cmake_minimum_required(VERSION 3.10)

project(cimbar_recv2)

set (SOURCES
	recv2.cpp
)

add_executable (
	cimbar_recv2
	${SOURCES}
)

target_link_libraries(cimbar_recv2

	cimbar_js
	extractor

	GL
	glfw
	${OPENCV_LIBS}
	opencv_videoio
)

add_custom_command(
	TARGET cimbar_recv2 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar_recv2> cimbar_recv2.dbg
	COMMAND ${CMAKE_STRIP} -g $<TARGET_FILE:cimbar_recv2>
)

