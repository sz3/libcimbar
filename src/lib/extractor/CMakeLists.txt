cmake_minimum_required(VERSION 3.10)

set(SOURCES
	Deskewer.cpp
	Deskewer.h
)

add_library(extractor STATIC ${SOURCES})

add_subdirectory(test)
