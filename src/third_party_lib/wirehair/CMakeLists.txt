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

if(MSVC)
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
endif()

include_directories(.)

add_library(wirehair STATIC ${LIB_SOURCE_FILES})
set_target_properties(wirehair PROPERTIES PUBLIC_HEADER wirehair.h)

add_executable(unit_test ${UNIT_TEST_SOURCE_FILES})
target_link_libraries(unit_test wirehair)

add_executable(gen_small_dseeds ${GEN_SMALL_DSEEDS})
target_link_libraries(gen_small_dseeds wirehair)

add_executable(gen_peel_seeds ${GEN_PEEL_SEEDS})
target_link_libraries(gen_peel_seeds wirehair)

add_executable(gen_most_dseeds ${GEN_MOST_DSEEDS})
target_link_libraries(gen_most_dseeds wirehair)

add_executable(gen_dcounts ${GEN_DCOUNTS})
target_link_libraries(gen_dcounts wirehair)

add_executable(gen_tables ${GEN_TABLES})

add_library(wirehair-shared SHARED ${LIB_SOURCE_FILES})
set_target_properties(wirehair-shared PROPERTIES VERSION 2)
set_target_properties(wirehair-shared PROPERTIES SOVERSION 2)
set_target_properties(wirehair-shared PROPERTIES PUBLIC_HEADER wirehair.h)

add_executable(unit_test_shared ${UNIT_TEST_SOURCE_FILES})
target_link_libraries(unit_test_shared wirehair-shared)

add_executable(gen_small_dseeds_shared ${GEN_SMALL_DSEEDS})
target_link_libraries(gen_small_dseeds_shared wirehair-shared)

add_executable(gen_peel_seeds_shared ${GEN_PEEL_SEEDS})
target_link_libraries(gen_peel_seeds_shared wirehair-shared)

add_executable(gen_most_dseeds_shared ${GEN_MOST_DSEEDS})
target_link_libraries(gen_most_dseeds_shared wirehair-shared)

add_executable(gen_dcounts_shared ${GEN_DCOUNTS})
target_link_libraries(gen_dcounts_shared wirehair-shared)

include(GNUInstallDirs)

install(TARGETS wirehair
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS wirehair-shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
