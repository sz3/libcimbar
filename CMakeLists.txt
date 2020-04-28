cmake_minimum_required(VERSION 3.10)

project ( libcimbar )
enable_testing()

set(CMAKE_CXX_FLAGS "-DLIBCIMBAR_PROJECT_ROOT=\\\"${libcimbar_SOURCE_DIR}\\\"")
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
	set(CMAKE_C_FLAGS "${CFLAGS} -Wall -std=c99")
	set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fpic -std=c++2a -static -static-libgcc -static-libstdc++")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -fpic -std=c++17")
	# -ftime-report
	# -ffast-math
endif()

if(NOT DEFINED OPENCV_LIBS)
	set(OPENCV_LIBS "opencv_core" "opencv_imgcodecs" "opencv_imgproc" "opencv_photo")
endif()

set (PROJECTS
   src/exe/cimbar
   src/exe/cimbar_scan
   src/exe/build_image_assets

   src/lib/bit_file
   src/lib/cimb_translator
   src/lib/encoder
   src/lib/extractor
   src/lib/image_hash
   src/lib/serialize
   src/lib/util

   src/third_party_lib/libcorrect
)

include_directories(
	${libcimbar_SOURCE_DIR}/src/lib
	${libcimbar_SOURCE_DIR}/src/third_party_lib
)

foreach(proj ${PROJECTS})
	add_subdirectory(${proj} build/${proj})
endforeach()

