cmake_minimum_required(VERSION 3.10)

project(concurrent_test)

set (SOURCES
	test.cpp
	thread_poolTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	concurrent_test
	${SOURCES}
)

add_test(concurrent_test concurrent_test)

target_link_libraries(concurrent_test

	pthread
)

