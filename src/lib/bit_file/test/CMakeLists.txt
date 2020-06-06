cmake_minimum_required(VERSION 2.6)

project(bit_file_test)

set (SOURCES
	test.cpp
	bitbufferTest.cpp
	bitreaderTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	bit_file_test
	${SOURCES}
)

add_test(bit_file_test bit_file_test)

