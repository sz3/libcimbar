cmake_minimum_required(VERSION 3.10)

set(SOURCES
	bitmaps.h
	image.h
	Common.cpp
	Common.h
	RayCommon.cpp
	RayCommon.h
)

add_library(graphics STATIC ${SOURCES})

target_link_libraries(graphics

	${RAYLIB}
	dl
	pthread
)
