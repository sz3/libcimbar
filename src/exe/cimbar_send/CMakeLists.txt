cmake_minimum_required(VERSION 3.10)

project(cimbar_send)

set (SOURCES
	send.cpp
)

add_executable (
	cimbar_send
	${SOURCES}
)

target_link_libraries(cimbar_send

	cimb_translator

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
	${libcimbar_UI_LIB}
)

add_custom_command(
	TARGET cimbar_send POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:cimbar_send> cimbar_send.dbg
	COMMAND ${CMAKE_STRIP} -g $<TARGET_FILE:cimbar_send>
)

install(
	TARGETS cimbar_send
	DESTINATION bin
)

