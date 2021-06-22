#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(system)

set(GAIA_SYSTEM_PUBLIC_INCLUDES
  "${GAIA_INC}"
)

message(VERBOSE "GAIA_SYSTEM_PUBLIC_INCLUDES=${GAIA_SYSTEM_PUBLIC_INCLUDES}")

set(GAIA_SYSTEM_PRIVATE_INCLUDES
  "${FLATBUFFERS_INC}"
  "${GAIA_REPO}/production/rules/event_manager/inc"
  "${GAIA_REPO}/production/db/inc/memory_manager"
  "${GAIA_REPO}/production/schemas/loader/inc"
  "${GAIA_REPO}/production/db/core/inc"
  "${GAIA_REPO}/production/db/core/tests/" # TODO remove once RI is supported in EDC
  )

message(VERBOSE "GAIA_SYSTEM_PRIVATE_INCLUDES=${GAIA_SYSTEM_PRIVATE_INCLUDES}")

# Our main target.
add_library(gaia_system STATIC src/gaia_system.cpp)

target_include_directories(gaia_system PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia_system PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia_system PUBLIC gaia_catalog gaia_direct gaia_rules)
set_target_properties(gaia_system PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_system PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

# The test needs gaia parser to load test DDLs.
set(TEST_GAIA_SYSTEM_INCLUDES
  ${GAIA_SYSTEM_PUBLIC_INCLUDES}
  ${GAIA_SYSTEM_PRIVATE_INCLUDES}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PARSER_GENERATED})

add_gtest(test_gaia_system "tests/test_gaia_system.cpp"
  "${TEST_GAIA_SYSTEM_INCLUDES}" "rt;gaia_system;gaia_schema_loader;edc_addr_book")

add_gtest(test_recovery "tests/test_recovery.cpp"
    "${TEST_GAIA_SYSTEM_INCLUDES}" "rt;gaia_system;gaia_schema_loader;edc_addr_book")
