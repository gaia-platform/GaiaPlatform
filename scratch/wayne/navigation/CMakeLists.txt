#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# starting point
set(GAIA_REPO ../../../../GaiaPlatform)

# Set the project name.
project(edc)

set(GAIA_INC ${GAIA_REPO}/production/inc)
set(FLATBUFFERS ${GAIA_REPO}/third_party/production/flatbuffers)
set(GAIA_COMPILE_FLAGS "-c -Wextra -ggdb -std=c++11")
set(GAIA_LINK_FLAGS "-ggdb")

set(GAIA_COMMON_INCLUDES 
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
# ${GAIA_REPO}/demos/airport_q1/inc
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${CMAKE_CURRENT_BINARY_DIR}
)
message(STATUS "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")

set (AIRIMPORT_INCLUDES
  ${GAIA_COMMON_INCLUDES}
  ${GAIA_REPO}/production/db/storage_engine/mock
)
include_directories(${AIRIMPORT_INCLUDES})

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/airport_generated.h"
  COMMAND ../${FLATBUFFERS}/build/flatc --cpp --gaiacpp --gen-mutable --gen-setters
    --gen-object-api --cpp-str-type gaia::common::nullable_string_t --cpp-str-flex-ctor
    -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/airport.fbs
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/airport.fbs
  COMMENT "Compiling airport.fbs"
)

add_custom_target(headers_generated
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/airport_generated.h")

add_executable(edc edc.cpp)
add_dependencies(edc headers_generated)

target_link_directories(edc
  PRIVATE
  ${GAIA_REPO}/production/build/direct_access
  ${GAIA_REPO}/production/build/rules/event_manager
  ${GAIA_REPO}/production/build/db/storage_engine
  ${GAIA_REPO}/production/build/system
)

target_link_libraries(edc PRIVATE rt gaia gaia_direct gaia_semock)

set_target_properties(edc PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(edc PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
