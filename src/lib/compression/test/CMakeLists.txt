cmake_minimum_required(VERSION 3.10)

project(compression_test)

set (SOURCES
	test.cpp
	zstd_compressorTest.cpp
	zstd_decompressorTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	compression_test
	${SOURCES}
)

add_test(compression_test compression_test)

target_link_libraries(compression_test
	zstd

	${CPPFILESYSTEM}
)

