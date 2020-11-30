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

if(MSVC)
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -march=native")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        add_definitions(-DLINUX_ARM=1)
    endif()
endif()

add_library(wirehair ${LIB_SOURCE_FILES})
set_target_properties(wirehair PROPERTIES VERSION 2)
set_target_properties(wirehair PROPERTIES SOVERSION 2)
target_include_directories(wirehair PUBLIC ${PROJECT_SOURCE_DIR}/include)

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

include(GNUInstallDirs)

install(TARGETS wirehair
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY python DESTINATION ${CMAKE_INSTALL_PREFIX})
