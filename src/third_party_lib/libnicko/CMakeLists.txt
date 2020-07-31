cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
set(SOURCES
	nicko.c
	nicko.h
)

add_library(nicko STATIC ${SOURCES})

