#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
project(demo_api)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)

include_directories(
    ../../../external/flatbuffers/include
    ../../../demos/cow_se/inc 
    ../inc
    ../perftimer
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/airport_generated.h"
    COMMAND flatc --cpp --gen-object-api  -o ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../demos/airport/flatbuffers/airport.fbs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../../demos/airport/flatbuffers/airport.fbs
    COMMENT "Compiling airport.fbs"
)

add_custom_target(whatever ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/airport_generated.h")

add_executable(demo_api
    StorageAPI.cpp
)
add_dependencies(demo_api whatever)

# add_executable(exp_api ExpAPI.cpp)

add_executable(nav_api NavAPI.cpp)
add_dependencies(nav_api whatever)

# add_dependencies(nav_api airport_generated.h)

set(GAIA_COMPILE_FLAGS "-c -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

target_link_libraries(demo_api 
    PRIVATE 
    rt
)

# target_link_libraries(exp_api PRIVATE rt)
target_link_libraries(nav_api PRIVATE rt)

set_target_properties(demo_api PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(demo_api PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
# set_target_properties(exp_api PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
# set_target_properties(exp_api PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
set_target_properties(nav_api PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(nav_api PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
