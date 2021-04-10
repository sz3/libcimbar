cmake_minimum_required(VERSION 2.6)

project(image_hash_test)

set (SOURCES
	test.cpp
	averageHashTest.cpp
	bitExtractorTest.cpp
	fuzzyAhashTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	image_hash_test
	${SOURCES}
)

add_test(image_hash_test image_hash_test)

target_link_libraries(image_hash_test
	cimb_translator

	${OPENCV_LIBS}
)

