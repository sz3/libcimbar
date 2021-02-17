cmake_minimum_required(VERSION 3.10)

project(chromatic_adaptation_test)

set (SOURCES
	test.cpp
	color_correctionTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	chromatic_adaptation_test
	${SOURCES}
)

add_test(chromatic_adaptation_test chromatic_adaptation_test)

target_link_libraries(chromatic_adaptation_test

	${OPENCV_LIBS}
)

