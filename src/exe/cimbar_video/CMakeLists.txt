cmake_minimum_required(VERSION 3.10)

project(cimbar_video)

set (SOURCES
	video.cpp
)

add_executable (
	cimbar_video
	${SOURCES}
)

target_link_libraries(cimbar_video

	cimb_translator

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
	opencv_highgui
)

add_custom_command(
	TARGET cimbar_video POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar_video> cimbar_video.dbg
	COMMAND ${CMAKE_STRIP} -g $<TARGET_FILE:cimbar_video>
)

install(
	TARGETS cimbar_video
	DESTINATION bin
)

