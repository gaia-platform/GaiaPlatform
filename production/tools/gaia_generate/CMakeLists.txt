#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(gaia_generate)

set(GAIA_GENERATE_INCLUDES
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_REPO}/production/build/catalog/catalog_manager/generated
  ${GAIA_REPO}/production/build/catalog/parser/generated
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${GAIA_REPO}/production/catalog/catalog_manager/inc
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/catalog
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
message(STATUS "GAIA_GENERATE_INCLUDES=${GAIA_GENERATE_INCLUDES}")

add_executable(gaia_generate src/main.cpp)
target_include_directories(gaia_generate PRIVATE ${GAIA_GENERATE_INCLUDES})
target_link_libraries(gaia_generate PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaia_generate PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_generate PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

add_gtest(test_gaia_generate "tests/test_gaia_generate.cpp" "${GAIA_GENERATE_INCLUDES}" "rt;gaia;gaia_catalog;gaia_parser")
