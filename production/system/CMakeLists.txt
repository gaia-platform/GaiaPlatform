#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(system)

set(GAIA_SYSTEM_PUBLIC_INCLUDES
  "${GAIA_INC}"
)
message(VERBOSE "GAIA_SYSTEM_PUBLIC_INCLUDES=${GAIA_SYSTEM_PUBLIC_INCLUDES}")

add_subdirectory(db)
add_subdirectory(db_rules)

set(GAIA_SYSTEM_PRIVATE_INCLUDES
  "${GAIA_REPO}/production/schemas/loader/inc"
  "${GAIA_REPO}/production/db/inc/core"
  "${GAIA_REPO}/production/db/core/tests/" # TODO remove once RI is supported in DAC
)
message(VERBOSE "GAIA_SYSTEM_PRIVATE_INCLUDES=${GAIA_SYSTEM_PRIVATE_INCLUDES}")

# The test needs gaia parser to load test DDLs.
set(TEST_GAIA_SYSTEM_INCLUDES
  ${GAIA_SYSTEM_PUBLIC_INCLUDES}
  ${GAIA_SYSTEM_PRIVATE_INCLUDES}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PARSER_GENERATED})

add_gtest(test_gaia_system "tests/test_gaia_system.cpp"
  "${TEST_GAIA_SYSTEM_INCLUDES}" "rt;gaia_system;gaia_schema_loader;dac_addr_book")

add_gtest(test_recovery "tests/test_recovery.cpp"
    "${TEST_GAIA_SYSTEM_INCLUDES}" "rt;gaia_system;gaia_schema_loader;dac_addr_book")
