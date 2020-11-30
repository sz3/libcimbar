cmake_minimum_required(VERSION 3.5)
project(wirehair)

set(CMAKE_CXX_STANDARD 11)

set(LIB_SOURCE_FILES
        wirehair.cpp
        include/wirehair/wirehair.h
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

if(NOT ANDROID)
    set(ARCH_NATIVE "-march=native")
endif()

if(MSVC)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_NATIVE}")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        add_definitions(-DLINUX_ARM=1)
    endif()
endif()

add_library(wirehair ${LIB_SOURCE_FILES})
set_target_properties(wirehair PROPERTIES VERSION 2)
set_target_properties(wirehair PROPERTIES SOVERSION 2)
target_include_directories(wirehair PUBLIC ${PROJECT_SOURCE_DIR}/include)

include(GNUInstallDirs)

