cmake_minimum_required(VERSION 3.10)

project(cimbar_js)

set (SOURCES
	cimbar_js.h
	cimbar_js.cpp
)

if(DEFINED USE_WASM)
	add_executable (
		cimbar_js
		${SOURCES}
	)
else()
	add_library (
		cimbar_js STATIC
		${SOURCES}
	)
endif()

target_link_libraries(cimbar_js

	cimb_translator

	correct_static
	wirehair
	zstd
	${OPENCV_LIBS}
)

### various emscripten shenanigans follow

# asm.js vs wasm
if(USE_WASM EQUAL "2")
	set (LINK_WASM_OPT "-s SINGLE_FILE=1")
else()
	set (LINK_WASM_OPT "-s WASM=1")
endif()

# extra link flags for emscripten
set (LINK_WASM_LIST
	--bind -Os
	${LINK_WASM_OPT}
	-s USE_WEBGL2=1
	-s MIN_WEBGL_VERSION=2
	-s MAX_WEBGL_VERSION=2
	-s USE_GLFW=3
	-s FILESYSTEM=0
	-s TOTAL_MEMORY=134217728
	-s EXPORTED_FUNCTIONS='["_render","_next_frame","_initialize_GL","_encode","_configure","_malloc","_free"]'
)
string(REPLACE ";" " " LINK_WASM_FLAGS "${LINK_WASM_LIST}")

if(DEFINED USE_WASM)
	set_target_properties( cimbar_js PROPERTIES LINK_FLAGS "${LINK_WASM_FLAGS}" )

	install(
		TARGETS cimbar_js
		DESTINATION ${libcimbar_SOURCE_DIR}/web
	)
	if(NOT USE_WASM EQUAL "2")
		install(
			FILES $<TARGET_FILE_DIR:cimbar_js>/cimbar_js.wasm
			DESTINATION ${libcimbar_SOURCE_DIR}/web
		)
	endif()

else()  # if no wasm, ignore all that and add in GLFW libraries
	target_link_libraries(cimbar_js
		GL
		glfw
	)
endif()
