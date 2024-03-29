###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(system)

set(GAIA_SYSTEM_PUBLIC_INCLUDES
  "${GAIA_INC}"
)
message(VERBOSE "GAIA_SYSTEM_PUBLIC_INCLUDES=${GAIA_SYSTEM_PUBLIC_INCLUDES}")

set(GAIA_SYSTEM_PRIVATE_INCLUDES
  "${GAIA_REPO}/production/schemas/loader/inc"
  "${GAIA_REPO}/production/db/inc/core"
  "${GAIA_REPO}/production/db/core/tests/" # TODO remove once RI is supported in DAC
)
message(VERBOSE "GAIA_SYSTEM_PRIVATE_INCLUDES=${GAIA_SYSTEM_PRIVATE_INCLUDES}")

# Build the database only static library.
add_library(gaia_system_db STATIC ./src/gaia_system.cpp)
configure_gaia_target(gaia_system_db)
target_include_directories(gaia_system_db PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia_system_db PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia_system_db PUBLIC gaia_catalog gaia_direct)

# Build the database + rules static library.
add_library(gaia_system STATIC ./src/gaia_system.cpp)
target_compile_definitions(gaia_system PRIVATE INCLUDE_RULES_ENGINE=1)
configure_gaia_target(gaia_system)
target_include_directories(gaia_system PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia_system PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia_system PUBLIC gaia_catalog gaia_direct gaia_rules)

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
