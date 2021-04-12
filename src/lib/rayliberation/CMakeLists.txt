cmake_minimum_required(VERSION 3.10)

set(SOURCES
	render_texture.cpp
	render_texture.h
	texture.cpp
	texture.h
	RayCommon.h
	RayCommon.cpp
	window_raylib.h
)

add_library(rayliberation STATIC ${SOURCES})

target_link_libraries(rayliberation
	cimb_translator

	${RAYLIB}
	dl
	pthread
)

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
