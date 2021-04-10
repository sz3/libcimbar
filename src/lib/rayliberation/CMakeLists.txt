cmake_minimum_required(VERSION 3.10)

set(SOURCES
	render_texture.h
	texture.h
	RayCommon.h
	RayCommon.cpp
)

add_library(rayliberation STATIC ${SOURCES})

target_link_libraries(rayliberation
	cimb_translator
)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
