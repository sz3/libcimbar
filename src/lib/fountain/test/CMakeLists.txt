cmake_minimum_required(VERSION 3.10)

project(fountain_test)

set (SOURCES
	test.cpp
	FountainEncodingTest.cpp
	FountainMetadataTest.cpp
	fountain_sinkTest.cpp
	fountain_streamTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	fountain_test
	${SOURCES}
)

add_test(fountain_test fountain_test)

target_link_libraries(fountain_test
	wirehair

	${CPPFILESYSTEM}
)

