#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(edc)

set(GAIA_INC ${GAIA_REPO}/production/inc)
set(FLATBUFFERS ${GAIA_REPO}/third_party/production/flatbuffers)
set(GAIA_COMPILE_FLAGS "-c -Wextra -ggdb -std=c++11")
set(GAIA_LINK_FLAGS "-ggdb")

set(GAIA_COMMON_INCLUDES 
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/internal/common
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
  OUTPUT "gaia_airport.h"
  COMMAND gaiac ${CMAKE_CURRENT_SOURCE_DIR}/airport.ddl
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/airport.ddl
  COMMENT "Compiling airport.ddl"
)

add_custom_target(headers_generated
  DEPENDS "gaia_airport.h"
)

add_executable(edc edc.cpp)
add_dependencies(edc headers_generated)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

add_library(gaia_semock SHARED IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

target_link_libraries(edc PRIVATE rt gaia gaia_direct gaia_semock)

set_target_properties(edc PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(edc PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
