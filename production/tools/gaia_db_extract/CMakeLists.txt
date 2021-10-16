#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(gaia_db_extract)

set(GAIA_DB_EXTRACT_INCLUDES
  ${FLATBUFFERS_INC}
  # ${GAIA_REPO}/production/db/core/src
  # ${GAIA_PROD_BUILD}/catalog/parser/generated
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  # ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${GAIA_INC}
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_DB_EXTRACT_INCLUDES=${GAIA_DB_EXTRACT_INCLUDES}")

add_executable(gaia_db_extract src/main.cpp src/gaia_db_extract.cpp)
configure_gaia_target(gaia_db_extract)
target_include_directories(gaia_db_extract PRIVATE ${GAIA_DB_EXTRACT_INCLUDES})
target_link_libraries(gaia_db_extract PRIVATE rt gaia_catalog gaia_parser gaia_db_client Threads::Threads)

add_gtest(test_gaia_db_extract
  "tests/test_gaia_db_extract.cpp;src/gaia_db_extract.cpp"
  "${GAIA_DB_EXTRACT_INCLUDES}"
  "rt;gaia_system;gaia_catalog;gaia_parser;edc_airport")
