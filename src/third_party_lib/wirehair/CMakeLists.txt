cmake_minimum_required(VERSION 3.5)
project(wirehair)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 11)

set(LIB_SOURCE_FILES
        wirehair.cpp
        wirehair.h
        gf256.cpp
        gf256.h
        WirehairCodec.cpp
        WirehairCodec.h
        WirehairTools.cpp
        WirehairTools.h
        )

set(UNIT_TEST_SOURCE_FILES
        test/SiameseTools.cpp
        test/SiameseTools.h
        test/UnitTest.cpp
        )

set(GEN_SMALL_DSEEDS
        test/SiameseTools.cpp
        test/SiameseTools.h
        tables/GenerateSmallDenseSeeds.cpp
        )

set(GEN_PEEL_SEEDS
        test/SiameseTools.cpp
        test/SiameseTools.h
        tables/GeneratePeelSeeds.cpp
        )

set(GEN_MOST_DSEEDS
        test/SiameseTools.cpp
        test/SiameseTools.h
        tables/GenerateMostDenseSeeds.cpp
        )

set(GEN_DCOUNTS
        test/SiameseTools.cpp
        test/SiameseTools.h
        tables/GenerateDenseCount.cpp
        )

set(GEN_TABLES
        test/SiameseTools.cpp
        test/SiameseTools.h
        tables/TableGenerator.cpp
        tables/HeavyRowGenerator.cpp
        tables/HeavyRowGenerator.h
        gf256.cpp
        gf256.h
        )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT ANDROID)
    set(ARCH_NATIVE "-march=native")
endif()

if(MSVC)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${ARCH_NATIVE}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 ${ARCH_NATIVE}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${ARCH_NATIVE}")
endif()

include_directories(.)

add_library(wirehair STATIC ${LIB_SOURCE_FILES})
set_target_properties(wirehair PROPERTIES PUBLIC_HEADER wirehair.h)

add_library(wirehair-shared SHARED ${LIB_SOURCE_FILES})
set_target_properties(wirehair-shared PROPERTIES VERSION 2)
set_target_properties(wirehair-shared PROPERTIES SOVERSION 2)
set_target_properties(wirehair-shared PROPERTIES PUBLIC_HEADER wirehair.h)

include(GNUInstallDirs)

install(TARGETS wirehair
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS wirehair-shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
