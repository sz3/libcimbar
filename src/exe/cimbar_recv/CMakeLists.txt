cmake_minimum_required(VERSION 3.10)

project(cimbar_recv)

set (SOURCES
	recv.cpp
)

add_executable (
	cimbar_recv
	${SOURCES}
)

target_link_libraries(cimbar_recv

	cimb_translator
	extractor

	correct_static
	wirehair
	zstd

	GL
	glfw
	${OPENCV_LIBS}
	opencv_videoio
)

add_custom_command(
	TARGET cimbar_recv POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar_recv> cimbar_recv.dbg
	COMMAND ${CMAKE_STRIP} -g $<TARGET_FILE:cimbar_recv>
)

install(
	TARGETS cimbar_recv
	DESTINATION bin
)

