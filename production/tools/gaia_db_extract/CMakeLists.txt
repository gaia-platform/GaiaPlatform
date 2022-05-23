###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(gaia_db_extract)

set(GAIA_DB_EXTRACT_TEST_INCLUDES
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
  ${JSON_INC}
  ${GAIA_REPO}/production/db/inc/core
  ${GAIA_REPO}/production/db/inc/payload_types
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_DB_EXTRACT_TEST_INCLUDES=${GAIA_DB_EXTRACT_TEST_INCLUDES}")

# The main target.
add_executable(gaia_db_extract
    src/main.cpp
    src/gaia_db_extract.cpp
    src/table_iterator.cpp
)
configure_gaia_target(gaia_db_extract)
target_include_directories(gaia_db_extract PRIVATE ${GAIA_DB_EXTRACT_TEST_INCLUDES})
target_link_libraries(gaia_db_extract PRIVATE rt gaia_catalog gaia_parser gaia_db_client Threads::Threads)

install(TARGETS gaia_db_extract DESTINATION ${CMAKE_INSTALL_BINDIR})

# Start unit tests.
add_gtest(test_gaia_db_extract
  "tests/test_gaia_db_extract.cpp;src/gaia_db_extract.cpp;src/table_iterator.cpp"
  "${GAIA_DB_EXTRACT_TEST_INCLUDES}"
  "rt;gaia_system;gaia_catalog;gaia_parser")

add_gtest(test_row_extract
  "tests/test_row_extract.cpp;src/gaia_db_extract.cpp;src/table_iterator.cpp"
  "${GAIA_DB_EXTRACT_TEST_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test;dac_addr_book;dac_amr_swarm")

add_gtest(test_row_field_values
  "tests/test_row_field_values.cpp;src/gaia_db_extract.cpp;src/table_iterator.cpp"
  "${GAIA_DB_EXTRACT_TEST_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test;dac_addr_book;dac_amr_swarm")
