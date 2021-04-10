cmake_minimum_required(VERSION 3.10)

project(cimb_translator_test)

set (SOURCES
	test.cpp
	AdjacentCellFinderTest.cpp
	CellTest.cpp
	CellDriftTest.cpp
	CellPositionsTest.cpp
	CimbDecoderTest.cpp
	CimbEncoderTest.cpp
	CimbReaderTest.cpp
	CimbWriterTest.cpp
	FloodDecodePositionsTest.cpp
	InterleaveTest.cpp
	LinearDecodePositionsTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	cimb_translator_test
	${SOURCES}
)

add_test(cimb_translator_test cimb_translator_test)

target_link_libraries(cimb_translator_test
	cimb_translator

	${OPENCV_LIBS}
)

