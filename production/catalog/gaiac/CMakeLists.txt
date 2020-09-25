#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)
enable_testing()

# Set the project name.
project(gaiac)

set(GAIA_CATALOG_TOOL_INCLUDES
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_REPO}/production/catalog/catalog_manager/inc
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/db
  ${GAIA_INC}/internal/db
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/public/system
  ${GAIA_PARSER_GENERATED}
)

message(VERBOSE "GAIA_CATALOG_TOOL_INCLUDES=${GAIA_CATALOG_TOOL_INCLUDES}")

add_executable(gaiac
  src/main.cpp
  ${FLATBUFFERS}/src/idl_gen_cpp.cpp
  ${FLATBUFFERS}/src/code_generators.cpp)
target_include_directories(gaiac PRIVATE ${GAIA_CATALOG_TOOL_INCLUDES})
add_dependencies(gaiac gaia_se_server)
target_link_libraries(gaiac PRIVATE rt gaia_catalog gaia_parser flatbuffers)
set_target_properties(gaiac PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaiac PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
