cmake_minimum_required(VERSION 2.6)

project(rayliberation_test)

set (SOURCES
	test.cpp
	render_textureTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	rayliberation_test
	${SOURCES}
)

add_test(rayliberation_test rayliberation_test)

target_link_libraries(rayliberation_test

	graphics

	${RAYLIB}
	dl
	pthread
)

