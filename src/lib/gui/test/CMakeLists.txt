cmake_minimum_required(VERSION 3.10)

project(gui_test)

set (SOURCES
	test.cpp
	window_glfwTest.cpp
)

include_directories(
	${libcimbar_SOURCE_DIR}/test
	${libcimbar_SOURCE_DIR}/test/lib
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable (
	gui_test
	${SOURCES}
)

add_test(gui_test gui_test)

target_link_libraries(gui_test

	glfw
	GL
	${OPENCV_LIBS}
	${CPPFILESYSTEM}
)

