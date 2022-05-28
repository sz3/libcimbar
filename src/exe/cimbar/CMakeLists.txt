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

	cimb_translator
	extractor

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
	${CPPFILESYSTEM}
)

add_custom_command(
	TARGET cimbar POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar> cimbar.dbg
	COMMAND ${CMAKE_STRIP} -g $<TARGET_FILE:cimbar>
)

install(
	TARGETS cimbar
	DESTINATION bin
)

