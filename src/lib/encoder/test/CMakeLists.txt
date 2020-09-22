cmake_minimum_required(VERSION 3.10)

project(encoder_test)

set (SOURCES
	test.cpp
	DecoderTest.cpp
	EncoderTest.cpp
	EncoderRoundTripTest.cpp
	aligned_streamTest.cpp
	reed_solomon_streamTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	encoder_test
	${SOURCES}
)

add_test(encoder_test encoder_test)

target_link_libraries(encoder_test

	cimb_translator

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
	${CPPFILESYSTEM}
)

