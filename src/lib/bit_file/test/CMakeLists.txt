cmake_minimum_required(VERSION 3.10)

project(bit_file_test)

set (SOURCES
	test.cpp
	bitbufferTest.cpp
	bitmatrixTest.cpp
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

target_link_libraries(bit_file_test

	${OPENCV_LIBS}
)
