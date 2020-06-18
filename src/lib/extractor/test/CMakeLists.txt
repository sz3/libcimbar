cmake_minimum_required(VERSION 3.10)

project(extractor_test)

set (SOURCES
	test.cpp
	CornersTest.cpp
	DeskewerTest.cpp
	ExtractorTest.cpp
	ScanStateTest.cpp
	ScannerTest.cpp
	SimpleCameraCalibrationTest.cpp
	UndistortTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	extractor_test
	${SOURCES}
)

add_test(extractor_test extractor_test)

target_link_libraries(extractor_test
	extractor

	${OPENCV_LIBS}
	${CPPFILESYSTEM}
)

