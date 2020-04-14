cmake_minimum_required(VERSION 3.10)

set(SOURCES
	format.h
)

add_custom_target(serialize SOURCES ${SOURCES})

