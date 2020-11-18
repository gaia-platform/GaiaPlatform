#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(event_manager)

set(GAIA_EVENT_MANAGER_INCLUDES
  ${GAIA_INC}
  ${GAIA_INTERNAL_INC}/common
  ${GAIA_INTERNAL_INC}/db
  ${GAIA_INTERNAL_INC}/catalog
  ${GAIA_INTERNAL_INC}/rules
  ${GAIA_RULES_INC}
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GAIA_GENERATED_SCHEMAS}
  tests
)

message(VERBOSE "GAIA_EVENT_MANAGER_INCLUDES=${GAIA_EVENT_MANAGER_INCLUDES}")

# main target
add_library(gaia_rules STATIC
  src/event_manager.cpp
  src/rule_thread_pool.cpp
  src/rule_context.cpp
  src/rule_checker.cpp
  src/rule_stats.cpp
  src/scheduler_stats.cpp
  src/rule_stats_manager.cpp
  src/event_manager_settings.cpp
)

add_dependencies(gaia_rules generate_rules_headers)
set_target_properties(gaia_rules PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(gaia_rules PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})
target_link_libraries(gaia_rules PRIVATE gaia_direct gaia_catalog)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_rules PROPERTY POSITION_INDEPENDENT_CODE ON)

# The rules integration and system init tests need gaia and logger configuration files.
configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")
configure_file("${PROJECT_SOURCE_DIR}/tests/invalid_gaia.conf" "${PROJECT_BINARY_DIR}/invalid_gaia.conf")
configure_file("${PROJECT_SOURCE_DIR}/tests/invalid_gaia_setting.conf" "${PROJECT_BINARY_DIR}/invalid_gaia_setting.conf")

# unit tests
add_gtest(test_auto_transaction "tests/test_auto_transaction.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_direct")
add_gtest(test_event_manager "tests/test_event_manager.cpp;tests/event_manager_test_helpers.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_system")
add_gtest(test_system_init "tests/test_system_init.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_system")
add_gtest(test_component_init "tests/test_component_init.cpp;tests/event_manager_test_helpers.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_common;gaia_rules;gaia_catalog")
add_gtest(test_rule_context "tests/test_rule_context.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_common;gaia_rules")
add_gtest(test_rule_checker "tests/test_rule_checker.cpp;"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_system")
add_gtest(test_rule_stats "tests/test_rule_stats.cpp;${GAIA_REPO}/production/common/src/debug_logger.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_system")

# The test needs gaia parser to load test DDLs.
set(TEST_RULE_INTEGRATION_INCLUDES
  ${GAIA_EVENT_MANAGER_INCLUDES}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PARSER_GENERATED})
add_gtest(test_rule_integration
  "tests/test_rule_integration.cpp;tests/event_manager_test_helpers.cpp"
  "${TEST_RULE_INTEGRATION_INCLUDES}" "rt;gaia_system;gaia_parser" "generate_addr_book_headers" ""
  "DDL_FILE=${GAIA_REPO}/production/schemas/test/addr_book/addr_book.ddl")
