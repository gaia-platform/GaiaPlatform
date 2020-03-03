#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
project(airport_data)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)

include_directories(
    ../../../third_party/production/flatbuffers/include 
    ../../../demos/cow_se/inc 
    ../inc
    ../perftimer
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/airport_generated.h"
    COMMAND flatc --cpp --gen-object-api --cpp-str-type gaia::string --cpp-str-flex-ctor -o ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../demos/airport/flatbuffers/airport.fbs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../../demos/airport/flatbuffers/airport.fbs
    COMMENT "Compiling airport.fbs"
)

add_custom_target(whatever ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/airport_generated.h")

add_executable(airimport
  DataLoader.cpp
)
add_dependencies(airimport whatever)

set(GAIA_COMPILE_FLAGS "-c -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

target_link_libraries(airimport
  PRIVATE
  rt
  uuid
)

set_target_properties(airimport PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(airimport PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
