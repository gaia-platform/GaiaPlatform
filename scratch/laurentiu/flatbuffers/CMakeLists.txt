#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(flatbuffers)

find_library(FLATBUFFERS_LIB flatbuffers)
if (NOT FLATBUFFERS_LIB)
    message(STATUS "Could not find flatbuffers library! Some targets will not be built!")
else()
    message(STATUS "Found flatbuffers library: ${FLATBUFFERS_LIB}!")

    # Flatbuffers C++ Tutorial.
    add_executable(Tutorial Tutorial.cpp Utils.cpp MonsterHelper.cpp)
    target_include_directories(Tutorial PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(Tutorial PRIVATE rt)
    set_target_properties(Tutorial PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
    set_target_properties(Tutorial PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

    # Field access test.
    add_executable(TestAccess TestAccess.cpp Utils.cpp MonsterHelper.cpp)
    target_include_directories(TestAccess PRIVATE ${PROJECT_SOURCE_DIR} )
    target_link_libraries(TestAccess PRIVATE rt ${FLATBUFFERS_LIB})
    set_target_properties(TestAccess PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
    set_target_properties(TestAccess PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

    # Versions test.
    add_executable(TestVersions TestVersions.cpp Utils.cpp)
    target_include_directories(TestVersions PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(TestVersions PRIVATE rt)
    set_target_properties(TestVersions PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
    set_target_properties(TestVersions PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

    # Advanced features test.
    add_executable(TestAdvanced TestAdvanced.cpp Utils.cpp MonsterHelper.cpp)
    target_include_directories(TestAdvanced PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(TestAdvanced PRIVATE rt)
    set_target_properties(TestAdvanced PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
    set_target_properties(TestAdvanced PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

    # Record test.
    add_executable(TestRecord TestRecord.cpp)
    target_include_directories(TestRecord PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(TestRecord PRIVATE rt)
    set_target_properties(TestRecord PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
    set_target_properties(TestRecord PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

    # Copy binary file required for Tutorial.
    configure_file(generated/monster_data.bin . COPYONLY)
    # Copy binary files required for Versions test.
    configure_file(generated/blobv1_data.bin . COPYONLY)
    configure_file(generated/blobv2_data1.bin . COPYONLY)
    configure_file(generated/blobv2_data2.bin . COPYONLY)
    # Copy binary schema required for TestAccess.
    configure_file(generated/monster.bfbs . COPYONLY)

    add_test(NAME test_flatbuffers_tutorial COMMAND Tutorial)
    add_test(NAME test_flatbuffers_access COMMAND TestAccess)
    add_test(NAME test_flatbuffers_versions COMMAND TestVersions)
    add_test(NAME test_flatbuffers_advanced COMMAND TestAdvanced)
    add_test(NAME test_record COMMAND TestRecord)
endif ()

find_library(FLATCCRT_LIB flatccrt)
if (NOT FLATCCRT_LIB)
    message(STATUS "Could not find flatccrt library! Some targets will not be built!")
else()
    message(STATUS "Found flatccrt library: ${FLATCCRT_LIB}!")

    # Flatbuffers C Tutorial.
    add_executable(Tutorial_C Tutorial.c)
    target_include_directories(Tutorial_C PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(Tutorial_C PRIVATE rt flatccrt)
    set_target_properties(Tutorial_C PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
    set_target_properties(Tutorial_C PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

    add_test(NAME test_flatbuffers_tutorial_c COMMAND Tutorial_C)
endif ()
