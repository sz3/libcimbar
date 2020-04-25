cmake_minimum_required(VERSION 3.10)

project(cimbar_decode)

set (SOURCES
	cimbar.cpp
)

add_executable (
	cimbar
	${SOURCES}
)

target_link_libraries(cimbar

-Wl,--start-group
	cimb_translator
	encoder
	extractor
-Wl,--end-group

	correct
	${OPENCV_LIBS}
)


