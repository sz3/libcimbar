cmake_minimum_required(VERSION 2.6)

project(util_test)

set (SOURCES
	test.cpp
	sampling_min_stackTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	util_test
	${SOURCES}
)

add_test(util_test util_test)

