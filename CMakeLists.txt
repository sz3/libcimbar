cmake_minimum_required(VERSION 3.7)

project ( libcimbar )
enable_testing()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${libcimbar_SOURCE_DIR}/dist" CACHE PATH "..." FORCE)
endif()

add_definitions("-DLIBCIMBAR_PROJECT_ROOT=\"${libcimbar_SOURCE_DIR}\"")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
	set(CMAKE_C_FLAGS "${CFLAGS} -Wall -std=c99 -Os -fPIC")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++17 -Wall -g -O2 -fPIC")
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

if(DEFINED STATIC_OPENCV_DIR)
	include_directories(${STATIC_OPENCV_DIR})
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(OPENCV4 REQUIRED opencv4)
	pkg_check_modules(GLFW3 REQUIRED glfw3)
	link_directories(${OPENCV4_STATIC_LIBRARY_DIRS} ${GLFW3_STATIC_LIBRARY_DIRS})

	# for static libc++
	#  -lc++ -lc++abi
	# _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -stdlib=libc++")
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib++ /usr/lib/llvm-11/lib/libc++.a /usr/lib/llvm-11/lib/libc++abi.a")
endif()

if(NOT DEFINED OPENCV_LIBS)
	set(OPENCV_LIBS "opencv_imgcodecs" "opencv_imgproc" "opencv_photo" "opencv_core" ${OPENCV4_STATIC_LIBRARIES})
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


