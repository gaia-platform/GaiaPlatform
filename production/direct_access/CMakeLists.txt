#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(direct_access)

set(GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES
  ${GAIA_INC}
)

message(VERBOSE "GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES=${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}")

set(GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/db
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${GAIA_REPO}/production/db/inc/expressions
  ${PROJECT_SOURCE_DIR}
)

message(VERBOSE "GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES=${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}")

# Our main target.
add_library(gaia_direct STATIC
  src/dac_base.cpp
  src/auto_transaction.cpp
)
configure_gaia_target(gaia_direct)
target_include_directories(gaia_direct PUBLIC ${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES})
target_include_directories(gaia_direct PRIVATE ${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES})
target_link_libraries(gaia_direct PUBLIC gaia_db_client)

# Start unit tests.
set(DIRECT_ACCESS_TEST_INCLUDES
  ${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}
  ${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}
)

add_gtest(test_direct_access "tests/test_direct_access.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_references "tests/test_references.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_iterator_conformance "tests/test_iterator_conformance.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_iterator "tests/test_iterator.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_incubator "tests/test_incubator.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_barn_storage")
add_gtest(test_expressions "tests/test_expressions.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_expressions_no_dac "tests/test_expressions_no_dac.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;flatbuffers")
add_gtest(test_one_to_one "tests/test_one_to_one.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_one_to_one")
add_gtest(test_one_to_many "tests/test_one_to_many.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_auto_connect "tests/test_auto_connect.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_airport")
add_gtest(test_default_db "tests/test_default_db.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac")

add_gtest(test_array_fields
  "tests/test_array_fields.cpp"
  "${DIRECT_ACCESS_TEST_INCLUDES}"
  "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")

add_gtest(test_optional_dac
  "tests/test_optional_dac.cpp"
  "${DIRECT_ACCESS_TEST_INCLUDES}"
  "rt;gaia_direct;gaia_db_catalog_test;dac_optional_sandbox")

add_gtest(test_insert_perf
  "tests/test_insertion_perf.cpp"
  "${DIRECT_ACCESS_TEST_INCLUDES};"
  "rt;gaia_direct;gaia_db_catalog_test;dac_insert_sandbox")

add_gtest(test_read_perf
  "tests/test_read_perf.cpp"
  "${DIRECT_ACCESS_TEST_INCLUDES};"
  "rt;gaia_direct;gaia_db_catalog_test;dac_insert_sandbox")

add_gtest(test_expressions_perf
  "tests/test_expressions_perf.cpp"
  "${DIRECT_ACCESS_TEST_INCLUDES}"
  "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
