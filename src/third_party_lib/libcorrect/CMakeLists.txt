cmake_minimum_required(VERSION 3.10)
project(Correct C)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(CheckCCompilerFlag)

if(MSVC)
set(LIBM "")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
else(MSVC)
set(LIBM "m")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -std=c99 -Wall")
check_c_compiler_flag(-Wpedantic COMPILER_SUPPORTS_WPEDANTIC)
if(COMPILER_SUPPORTS_WPEDANTIC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpedantic")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0 -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie,")
else()
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
    else()
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL "Profiling")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g3")
  endif()
endif()
endif(MSVC)

find_library(FEC fec)
CHECK_LIBRARY_EXISTS(FEC dotprod "" HAVE_LIBFEC)

if(NOT CMAKE_CROSSCOMPILING)
  # Check if host machine can compile with SSE 4.1 intrinsic
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_DEFINITIONS -march=native)
  check_c_source_compiles("
    #include <x86intrin.h>
    int main() {
      __m128i a;
      __m128i b;
      __m128i c = _mm_min_epu16(a, b);
      return 0;
    }" HAVE_SSE)
  cmake_pop_check_state()
endif()

if(HAVE_SSE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.1")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(src)

set(INSTALL_HEADERS "${PROJECT_BINARY_DIR}/include/correct.h")

add_custom_target(correct-h ALL COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/include/correct.h ${PROJECT_BINARY_DIR}/include/correct.h)

if(HAVE_SSE)
  set(correct_obj_files $<TARGET_OBJECTS:correct-reed-solomon> $<TARGET_OBJECTS:correct-convolutional> $<TARGET_OBJECTS:correct-convolutional-sse>)
  set(INSTALL_HEADERS ${INSTALL_HEADERS} ${PROJECT_BINARY_DIR}/include/correct-sse.h)
  add_custom_target(correct-sse-h ALL COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/include/correct-sse.h ${PROJECT_BINARY_DIR}/include/correct-sse.h)
else()
  set(correct_obj_files $<TARGET_OBJECTS:correct-reed-solomon> $<TARGET_OBJECTS:correct-convolutional>)
endif()
add_library(correct_static ${correct_obj_files})
set_target_properties(correct_static PROPERTIES OUTPUT_NAME "correct")
if(HAVE_SSE)
  target_compile_definitions(correct_static PUBLIC HAVE_SSE=1)
endif()

#add_subdirectory(util)
#add_subdirectory(tests)
#add_subdirectory(tools)
# add_subdirectory(benchmarks)


