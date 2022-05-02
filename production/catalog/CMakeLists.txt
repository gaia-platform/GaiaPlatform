#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(catalog)

# The catalog.cpp is generated. Look into schemas/system/catalog/CMakeLists.txt to re-generate the code.
add_library(dac_catalog STATIC
    src/gaia_catalog.cpp
)

configure_gaia_target(dac_catalog)
target_include_directories(dac_catalog PRIVATE ${GAIA_INC})
target_include_directories(dac_catalog PRIVATE ${GAIA_INC}/gaia_internal/catalog)
target_include_directories(dac_catalog PUBLIC ${FLATBUFFERS_INC})
target_link_libraries(dac_catalog PUBLIC gaia_direct)

set(GAIA_CATALOG_PUBLIC_INCLUDES
  ${GAIA_INC}
)

message(VERBOSE "GAIA_CATALOG_PUBLIC_INCLUDES=${GAIA_CATALOG_PUBLIC_INCLUDES}")

set(GAIA_CATALOG_PRIVATE_INCLUDES
  ${GAIA_REPO}/production/db/inc/core
  ${GAIA_REPO}/production/db/inc/index
  ${GAIA_PROD_BUILD}/catalog/parser/generated
  ${PROJECT_SOURCE_DIR}/inc
  ${PROJECT_SOURCE_DIR}/parser/inc
)

message(VERBOSE "GAIA_CATALOG_PRIVATE_INCLUDES=${GAIA_CATALOG_PRIVATE_INCLUDES}")

# These files is downloaded with ExternalProject_Add in third_party/production/flatbuffers.
# CMake is not able to detect this dependency and fails at config time.
# The GENERATED property makes cmake to skip the source check at config time.
set_source_files_properties(${FLATBUFFERS_SRC}/src/code_generators.cpp PROPERTIES GENERATED TRUE)

add_library(gaia_catalog STATIC
  src/ddl_execution.cpp
  src/ddl_executor.cpp
  src/fbs_generator.cpp
  src/json_generator.cpp
  src/fdw_ddl_generator.cpp
  src/catalog.cpp
  src/catalog_facade.cpp
  src/gaiac_catalog_facade.cpp
  src/gaiat_catalog_facade.cpp
  src/dac_generator.cpp
  src/gaia_generate.cpp
  ${FLATBUFFERS_SRC}/src/code_generators.cpp)

configure_gaia_target(gaia_catalog)
target_include_directories(gaia_catalog PUBLIC ${GAIA_CATALOG_PUBLIC_INCLUDES})
target_include_directories(gaia_catalog PRIVATE ${GAIA_CATALOG_PRIVATE_INCLUDES})
target_link_libraries(gaia_catalog PRIVATE gaia_parser gaia_direct flatbuffers)
target_link_libraries(gaia_catalog PUBLIC dac_catalog)

add_gtest(test_ddl_executor
  "tests/test_ddl_executor.cpp"
  "${GAIA_CATALOG_PUBLIC_INCLUDES};${GAIA_CATALOG_PRIVATE_INCLUDES}"
  "rt;gaia_system;gaia_catalog;gaia_db_catalog_test;dac_addr_book"
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

set(TEST_GAIA_EXECUTION_INCLUDES
    ${GAIA_CATALOG_PUBLIC_INCLUDES}
    ${GAIA_CATALOG_PRIVATE_INCLUDES}
    ${GAIA_PROD_BUILD}/catalog/parser/generated
    ${PROJECT_SOURCE_DIR}/parser/inc
)

add_gtest(test_ddl_execution
  "tests/test_ddl_execution.cpp"
  "${TEST_GAIA_EXECUTION_INCLUDES}"
  "rt;gaia_system;gaia_catalog;gaia_db_catalog_test"
  gaia_catalog)

add_gtest(test_gaiac_generation
    "tests/test_gaiac_generation.cpp"
    "${TEST_GAIA_EXECUTION_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;dac_airport")

add_gtest(test_rule_tables
    "tests/test_rule_tables.cpp"
    "${TEST_GAIA_EXECUTION_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;")

add_subdirectory(gaiac)
add_subdirectory(parser)
