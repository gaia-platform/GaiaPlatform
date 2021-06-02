#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(catalog)

set(GAIA_CATALOG_PUBLIC_INCLUDES
  ${GAIA_INC}
)

message(VERBOSE "GAIA_CATALOG_PUBLIC_INCLUDES=${GAIA_CATALOG_PUBLIC_INCLUDES}")

set(GAIA_CATALOG_PRIVATE_INCLUDES
  ${GAIA_REPO}/production/db/core/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_CATALOG_PRIVATE_INCLUDES=${GAIA_CATALOG_PRIVATE_INCLUDES}")

add_library(gaia_catalog STATIC
  src/ddl_executor.cpp
  src/fbs_generator.cpp
  src/json_generator.cpp
  src/fdw_ddl_generator.cpp
  src/catalog.cpp
  src/gaia_generate.cpp
  src/gaia_generate2.cpp
  src/code_generator.cpp
  src/catalog_facade.cpp
  ${FLATBUFFERS}/src/code_generators.cpp)
set_target_properties(gaia_catalog PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_catalog PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

target_include_directories(gaia_catalog PUBLIC ${GAIA_CATALOG_PUBLIC_INCLUDES})
target_include_directories(gaia_catalog PRIVATE ${GAIA_CATALOG_PRIVATE_INCLUDES})
target_link_libraries(gaia_catalog PRIVATE gaia_direct flatbuffers)
set_property (TARGET gaia_catalog PROPERTY POSITION_INDEPENDENT_CODE ON)

add_gtest(test_ddl_executor
  "tests/test_ddl_executor.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia_system;gaia_catalog;gaia_db_catalog_test"
  gaia_catalog)

add_gtest(test_fbs_generation
  "tests/test_fbs_generation.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia_system;gaia_catalog"
  gaia_catalog)

add_gtest(test_json_generation
  "tests/test_json_generation.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia_system;gaia_catalog"
  gaia_catalog)

add_gtest(test_fdw_ddl_generation
  "tests/test_fdw_ddl_generation.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia_system;gaia_catalog"
  gaia_catalog)

add_gtest(test_edc_generation
    "tests/test_edc_generation.cpp;tests/addr_book.cpp"
    "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
    "rt;gaia_system;gaia_catalog;gaia_db_catalog_test"
    gaia_catalog)

add_subdirectory(gaiac)
add_subdirectory(parser)
