cmake_minimum_required(VERSION 3.10)

set(SOURCES
	bitmaps.h
	res.cpp
	res.h
)

add_library(res STATIC ${SOURCES})

