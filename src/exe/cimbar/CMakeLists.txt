cmake_minimum_required(VERSION 3.7)

project(cimbar_decode)

set (SOURCES
	cimbar.cpp
)

add_executable (
	cimbar
	${SOURCES}
)

target_link_libraries(cimbar

	cimb_translator
	extractor

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
)

add_custom_command(
	TARGET cimbar POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar> cimbar.dbg
	COMMAND strip -g $<TARGET_FILE:cimbar>
)

install(
	TARGETS cimbar
	DESTINATION bin
)

