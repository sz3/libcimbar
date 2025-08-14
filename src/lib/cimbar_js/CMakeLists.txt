cmake_minimum_required(VERSION 3.10)

project(cimbar_js)

set (SOURCES
	cimbar_js.h
	cimbar_js.cpp
)

# only if we're using wasm
if(NOT USE_WASM EQUAL "2")
set(SOURCES
	${SOURCES}

	cimbar_recv_js.h
	cimbar_recv_js.cpp
	cimbar_zstd_js.h
	cimbar_zstd_js.cpp
)
endif()

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
	extractor

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

set (LINK_WASM_EXPORTED_FUNCTIONS
	"_malloc",
	"_free",
	"_cimbare_render",
	"_cimbare_next_frame",
	"_cimbare_encode",
	"_cimbare_configure",
	"_cimbare_get_aspect_ratio"
)

if(NOT USE_WASM EQUAL "2")
set(LINK_WASM_EXPORTED_FUNCTIONS
	${LINK_WASM_EXPORTED_FUNCTIONS}

	,"_cimbard_get_report",
	"_cimbard_scan_extract_decode",
	"_cimbard_fountain_decode",
	"_cimbard_get_bufsize",
	"_cimbard_get_filesize",
	"_cimbard_get_filename",
	"_cimbard_finish_copy",
	"_cimbard_configure_decode",
	"_cimbarz_init_decompress",
	"_cimbarz_get_bufsize",
	"_cimbarz_decompress_read"
)
endif()

# extra link flags for emscripten
set (LINK_WASM_LIST
	--bind -Os
	${LINK_WASM_OPT}
	-s USE_WEBGL2=1
	-s MIN_WEBGL_VERSION=2
	-s MAX_WEBGL_VERSION=2
	-s USE_GLFW=3
	-s WASM_BIGINT=1
	-s FILESYSTEM=0
	-s TOTAL_MEMORY=134217728
	-s EXPORTED_FUNCTIONS='[${LINK_WASM_EXPORTED_FUNCTIONS}]'
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

if(NOT DEFINED DISABLE_TESTS)
	add_subdirectory(test)
endif()
