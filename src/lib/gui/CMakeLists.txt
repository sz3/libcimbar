cmake_minimum_required(VERSION 3.10)

set(SOURCES
	gl_2d_display.h
	gl_program.h
	gl_shader.h
	mat_to_gl.h
	shaky_cam.h
	window_cvhighgui.h
	window_glfw.h
	window.h
	window_interface.h

)

add_library(gui INTERFACE)

if(NOT DEFINED DISABLE_TESTS AND DEFINED USE_GLFW)
	add_subdirectory(test)
endif()

