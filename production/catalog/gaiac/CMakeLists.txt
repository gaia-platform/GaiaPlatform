#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(gaiac)

set(GAIA_CATALOG_TOOL_INCLUDES
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/public/system
  ${GAIA_PARSER_GENERATED_OUTPUTS}
)
message(STATUS "GAIA_CATALOG_TOOL_INCLUDES=${GAIA_CATALOG_TOOL_INCLUDES}")

add_executable(gaiac src/main.cpp)
target_include_directories(gaiac PRIVATE ${GAIA_CATALOG_TOOL_INCLUDES})
target_link_libraries(gaiac PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaiac PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaiac PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
