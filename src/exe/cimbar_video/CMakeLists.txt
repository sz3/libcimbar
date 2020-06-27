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

	correct
	wirehair
	${OPENCV_LIBS}
	opencv_highgui
)


