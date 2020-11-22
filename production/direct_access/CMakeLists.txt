#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(direct_access)

set(GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES
  ${GAIA_INC}
  ${GAIA_GENERATED_SCHEMAS}
)

message(VERBOSE "GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES=${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}")

set(GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES
  ${GAIA_INTERNAL_INC}/common
  ${GAIA_INTERNAL_INC}/db
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}
)

message(VERBOSE "GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES=${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}")

# Our main target.
add_library(gaia_direct STATIC
  src/gaia_base.cpp
  src/auto_transaction.cpp
)

target_include_directories(gaia_direct PUBLIC ${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES})
target_include_directories(gaia_direct PRIVATE ${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES})
target_link_libraries(gaia_direct PUBLIC gaia_se_client)
set_target_properties(gaia_direct PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_direct PROPERTY POSITION_INDEPENDENT_CODE ON)

# Start unit tests.
set(DIRECT_ACCESS_TEST_INCLUDES
  ${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}
  ${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}
)

add_gtest(test_direct_access "tests/test_direct_access.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test" "generate_addr_book_headers")
add_gtest(test_references "tests/test_references.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test" "generate_addr_book_headers")
add_gtest(test_iterators "tests/test_iterators.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test" "generate_addr_book_headers")
add_gtest(test_incubator "tests/test_incubator.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test" "generate_incubator_headers")
