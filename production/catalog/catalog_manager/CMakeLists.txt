#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)
enable_testing()

project(catalog_manager)

set(GAIA_CATALOG_PUBLIC_INCLUDES
  ${GAIA_INC}/internal/catalog
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/public/common
)

# message(STATUS "GAIA_CATALOG_PUBLIC_INCLUDES=${GAIA_CATALOG_PUBLIC_INCLUDES}")

set(GAIA_CATALOG_PRIVATE_INCLUDES
  ${GAIA_INC}/public/db
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/db
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
)

# message(STATUS "GAIA_CATALOG_PRIVATE_INCLUDES=${GAIA_CATALOG_PRIVATE_INCLUDES}")

# main target
add_library(gaia_catalog STATIC
  src/catalog_manager.cpp
  src/fbs_generator.cpp
  src/gaia_generate.cpp
  ${FLATBUFFERS}/src/code_generators.cpp)
set_target_properties(gaia_catalog PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(gaia_catalog PUBLIC ${GAIA_CATALOG_PUBLIC_INCLUDES})
target_include_directories(gaia_catalog PRIVATE ${GAIA_CATALOG_PRIVATE_INCLUDES})
target_link_libraries(gaia_catalog PRIVATE gaia_direct flatbuffers)
set_property (TARGET gaia_catalog PROPERTY POSITION_INDEPENDENT_CODE ON)

add_gtest(test_catalog_manager
  "tests/test_catalog_manager.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia;gaia_catalog"
  gaia_catalog)

add_gtest(test_fbs_generation
  "tests/test_fbs_generation.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia;gaia_catalog"
  gaia_catalog)
