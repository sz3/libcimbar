cmake_minimum_required(VERSION 3.10)

project ( libcimbar )
enable_testing()

set(CMAKE_CXX_FLAGS "-DLIBCIMBAR_PROJECT_ROOT=\\\"${libcimbar_SOURCE_DIR}\\\" -DEIGEN_MPL2_ONLY -std=c++17")
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
	set(CMAKE_C_FLAGS "${CFLAGS} -Wall -std=c99 -Os")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -fpic")
endif()

if(NOT DEFINED OPENCV_LIBS)
	set(OPENCV_LIBS "opencv_core" "opencv_imgcodecs" "opencv_imgproc" "opencv_photo")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        set(CPPFILESYSTEM "stdc++fs")
    endif()
endif()

set (PROJECTS
	src/exe/cimbar
	src/exe/cimbar_scan
	src/exe/cimbar_video
	src/exe/build_image_assets

	src/lib/bit_file
	src/lib/cimb_translator
	src/lib/encoder
	src/lib/extractor
	src/lib/fountain
	src/lib/image_hash
	src/lib/serialize
	src/lib/util

	src/third_party_lib/base91
	src/third_party_lib/cxxopts
	src/third_party_lib/intx
	src/third_party_lib/libcorrect
	src/third_party_lib/libpopcnt
	src/third_party_lib/wirehair
)

include_directories(
	${libcimbar_SOURCE_DIR}/src/lib
	${libcimbar_SOURCE_DIR}/src/third_party_lib
)

foreach(proj ${PROJECTS})
	add_subdirectory(${proj} build/${proj})
endforeach()

