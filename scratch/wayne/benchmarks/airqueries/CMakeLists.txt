#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(airqueries)

set(GAIA_REPO /home/wayne/src/github.com/gaia-platform/GaiaPlatform)
set(GAIA_INC ${GAIA_REPO}/production/inc)
set(GAIA_COMPILE_FLAGS "-c -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

set(GAIA_COMMON_INCLUDES 
    ${GAIA_INC}/public/common
    ${GAIA_INC}/internal/common
    # ${GAIA_INC}/public/rules
    # ${GAIA_RULES_INC}
    ${GAIA_REPO}/scratch/benchmarks/inc
    ${GAIA_REPO}/third_party/production/flatbuffers/include
)
message(STATUS "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")

set (AIRQUERIES_INCLUDES
    ${GAIA_COMMON_INCLUDES}
    ${GAIA_REPO}/production/db/storage_engine/mock
)
include_directories(${AIRQUERIES_INCLUDES})

add_executable(airqueries air_queries.cpp)
target_link_directories(airqueries
  PRIVATE
  ${CMAKE_BINARY_DIR}/../../../../../production/build/direct_access
  ${CMAKE_BINARY_DIR}/../../../../../production/build/rules/event_manager
  ${CMAKE_BINARY_DIR}/../../../../../production/build/db/storage_engine
  ${CMAKE_BINARY_DIR}/../../../../../production/build/system
)


target_link_libraries(airqueries PRIVATE rt gaia gaia_direct gaia_semock)

set_target_properties(airqueries PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(airqueries PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
