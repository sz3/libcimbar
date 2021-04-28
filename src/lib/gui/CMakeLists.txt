cmake_minimum_required(VERSION 3.10)

add_library(gui INTERFACE)

if(NOT DEFINED DISABLE_TESTS AND DEFINED USE_GLFW)
	add_subdirectory(test)
endif()

