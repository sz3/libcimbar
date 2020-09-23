cmake_minimum_required(VERSION 3.10)

project(cimbar_js)

set (SOURCES
	wasm.cpp
)

add_executable (
	cimbar_js
	${SOURCES}
)

target_link_libraries(cimbar_js

	cimb_translator

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
)

set_target_properties( cimbar_js PROPERTIES LINK_FLAGS "-s DEMANGLE_SUPPORT=1 --bind -s SINGLE_FILE=1 -Os -s FULL_ES2=1 -s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s USE_GLFW=3 -s FILESYSTEM=0 -s TOTAL_MEMORY=134217728 -s EXPORTED_FUNCTIONS='[\"_render\", \"_initialize_GL\", \"_encode\"]'" )

install(
	TARGETS cimbar_js
	DESTINATION ${libcimbar_SOURCE_DIR}/web
)
