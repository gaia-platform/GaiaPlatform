#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(system)

set(GAIA_SYSTEM_PUBLIC_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/public/db"
  "${GAIA_INC}/public/catalog"
  "${GAIA_INC}/public/direct_access"
  "${GAIA_INC}/public/system"
  "${GAIA_INC}/public/rules"
  "${GAIA_INC}/internal/rules"
  "${GAIA_GENERATED_SCHEMAS}"
)

message(VERBOSE "GAIA_SYSTEM_PUBLIC_INCLUDES=${GAIA_SYSTEM_PUBLIC_INCLUDES}")

set(GAIA_SYSTEM_PRIVATE_INCLUDES
  "${GAIA_INC}/internal/common"
  "${GAIA_INC}/internal/db"
  "${FLATBUFFERS_INC}"
  "${GAIA_REPO}/production/rules/event_manager/inc"
  "${GAIA_REPO}/production/db/storage_engine/tests/" # TODO remove once RI is supported in EDC
  )

message(VERBOSE "GAIA_SYSTEM_PRIVATE_INCLUDES=${GAIA_SYSTEM_PRIVATE_INCLUDES}")

# Our main target.
add_library(gaia_system STATIC src/gaia_system.cpp)

target_include_directories(gaia_system PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia_system PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia_system PUBLIC gaia_catalog gaia_direct gaia_rules)
set_target_properties(gaia_system PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_system PROPERTY POSITION_INDEPENDENT_CODE ON)

# The test needs gaia parser to load test DDLs.
set(TEST_GAIA_SYSTEM_INCLUDES
  ${GAIA_SYSTEM_PUBLIC_INCLUDES}
  ${GAIA_SYSTEM_PRIVATE_INCLUDES}
  "${GAIA_INC}/internal/catalog"
  "${GAIA_REPO}/production/catalog/parser/inc"
  ${GAIA_PARSER_GENERATED})
add_gtest(test_gaia_system "tests/test_gaia_system.cpp"
  "${TEST_GAIA_SYSTEM_INCLUDES}" "rt;gaia_system;gaia_parser" "generate_addr_book_headers" ""
  "DDL_FILE=${GAIA_REPO}/production/schemas/test/addr_book/addr_book.ddl")

# Recovery tests. Not to be run with ctest.
add_executable(test_recovery "tests/test_recovery.cpp")
target_include_directories(test_recovery PRIVATE "${GAIA_SYSTEM_PUBLIC_INCLUDES};${GAIA_SYSTEM_PRIVATE_INCLUDES};${GOOGLE_TEST_INC}")
add_dependencies(test_recovery "generate_addr_book_headers")
target_link_libraries(test_recovery PRIVATE rt gaia_system gtest_main)
set_target_properties(test_recovery PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(test_recovery PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
set_target_properties(test_recovery PROPERTIES CXX_CLANG_TIDY "")
