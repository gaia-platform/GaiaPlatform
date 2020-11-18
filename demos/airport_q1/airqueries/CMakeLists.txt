#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# starting point
set(GAIA_REPO ../../..)

# Set the project name.
project(airqueries)

set(GAIA_INC ${GAIA_REPO}/production/inc)
set(FLATBUFFERS ${GAIA_REPO}/third_party/production/flatbuffers)
set(GAIA_COMPILE_FLAGS "-c -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

set(GAIA_COMMON_INCLUDES 
  ${GAIA_INC}
  ${GAIA_INC}/gaia_internal/common
  ${GAIA_REPO}/demos/airport_q1/inc
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${CMAKE_CURRENT_BINARY_DIR}
)
message(STATUS "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")

set (AIRQUERIES_INCLUDES
  ${GAIA_COMMON_INCLUDES}
  ${GAIA_REPO}/production/db/storage_engine/mock
)
include_directories(${AIRQUERIES_INCLUDES})

# This command produces two headers in the build directory. This program (airqueries)
# modifies airport_q1_gaia_generated.h so that it points to a specialized gaia_object.hpp
# version called gaia_object_q1_demo.hpp in the airport_q1/inc/ directory. The specialized
# code was used for this demo only and the additional methods it contains will not be
# supported in ongoing versions of gaia_object.hpp.
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/airport_q1_generated.h"
  COMMAND ../${FLATBUFFERS}/build/flatc --cpp --gaiacpp --gen-mutable --gen-setters
    --gen-object-api --cpp-str-type gaia::common::nullable_string_t --cpp-str-flex-ctor
    -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../inc/airport_q1.fbs
  COMMAND sed "s/gaia_object.hpp/gaia_object_q1_demo.hpp/" -i "${CMAKE_CURRENT_BINARY_DIR}/airport_q1_gaia_generated.h"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../inc/airport_q1.fbs
  COMMENT "Compiling airport_q1.fbs"
)

add_custom_target(headers_generated
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/airport_q1_generated.h")

add_executable(airqueries air_queries.cpp)
add_dependencies(airqueries headers_generated)

target_link_directories(airqueries
  PRIVATE
  ${GAIA_REPO}/production/build/direct_access
  ${GAIA_REPO}/production/build/rules/event_manager
  ${GAIA_REPO}/production/build/db/storage_engine
  ${GAIA_REPO}/production/build/system
)

target_link_libraries(airqueries PRIVATE rt gaia gaia_direct gaia_semock)

set_target_properties(airqueries PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(airqueries PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
