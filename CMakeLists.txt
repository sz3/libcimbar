cmake_minimum_required(VERSION 3.10)

project ( libcimbar )
enable_testing()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${libcimbar_SOURCE_DIR}/dist" CACHE PATH "..." FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
add_definitions("-DLIBCIMBAR_PROJECT_ROOT=\"${libcimbar_SOURCE_DIR}\"")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
	set(CMAKE_C_FLAGS "${CFLAGS} -Wall -std=c99 -Os -fPIC")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Wall -g -O2 -fPIC")
endif()

if(DEFINED USE_WASM)
	set(DISABLE_TESTS true)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGF256_TARGET_MOBILE")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os")

	file( GLOB OPENCV_LIBS "${OPENCV_DIR}/opencv-build-wasm/build_wasm/lib/*.a" )
	file( GLOB opencv_include_modules "${OPENCV_DIR}/modules/*/include" )
	include_directories(
		${OPENCV_DIR}/include/
		${OPENCV_DIR}/opencv-build-wasm/build_wasm/
		${opencv_include_modules}
	)
endif()

if(NOT DEFINED OPENCV_LIBS)
	set(OPENCV_LIBS "opencv_core" "opencv_imgcodecs" "opencv_imgproc" "opencv_photo")
endif()

if(NOT DEFINED CPPFILESYSTEM)
	set(CPPFILESYSTEM "stdc++fs")
endif()

set (PROJECTS
	src/lib/bit_file
	src/lib/chromatic_adaptation
	src/lib/cimb_translator
	src/lib/cimbar_js
	src/lib/compression
	src/lib/encoder
	src/lib/fountain
	src/lib/gui
	src/lib/image_hash
	src/lib/res
	src/lib/serialize
	src/lib/util

	src/third_party_lib/base91
	src/third_party_lib/cxxopts
	src/third_party_lib/intx
	src/third_party_lib/libcorrect
	src/third_party_lib/libpopcnt
	src/third_party_lib/wirehair
	src/third_party_lib/zstd
)

if(NOT DEFINED USE_WASM)
set(PROJECTS
	${PROJECTS}
	src/lib/extractor

	src/exe/cimbar
	src/exe/cimbar_extract
	src/exe/cimbar_send
	src/exe/build_image_assets
)
endif()

include_directories(
	${libcimbar_SOURCE_DIR}/src/lib
	${libcimbar_SOURCE_DIR}/src/third_party_lib
)

foreach(proj ${PROJECTS})
	add_subdirectory(${proj} build/${proj})
endforeach()


