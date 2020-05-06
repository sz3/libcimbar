cmake_minimum_required(VERSION 3.10)

set(SOURCES
	monitor.h
	thread_pool.h
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(concurrent SOURCES ${SOURCES})

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
