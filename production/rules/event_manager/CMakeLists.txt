#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)
enable_testing()

# Set the project name.
project(event_manager)

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/rules/event_manager/generated")

set(GAIA_EVENT_MANAGER_INCLUDES
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/db
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/system
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/db
  ${GAIA_INC}/internal/catalog
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/internal/rules
  ${GAIA_RULES_INC}
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GENERATED_OUTPUTS}
  ${GAIA_CATALOG_GENERATED}
)
message(STATUS "GAIA_EVENT_MANAGER_INCLUDES=${GAIA_EVENT_MANAGER_INCLUDES}")

set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

# Generate the headers from the Gaia DDL
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_event_log.h
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS}
    ${CMAKE_CURRENT_SOURCE_DIR}/schema/event_log.ddl
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/schema/event_log.ddl
  COMMENT "Compiling event_log.ddl."
)

add_custom_target(generate_event_logs
    DEPENDS ${GENERATED_OUTPUTS}/gaia_event_log.h
)

# Generate our integration test "rules db" by concatenating
# schemas together.
gaia_make_db_ddl(${CMAKE_CURRENT_SOURCE_DIR}/tests/schema/addr_book.ddl ${GENERATED_OUTPUTS}/rules_db.ddl)

# Generate the headers for the integration test
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_rules_db.h
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS}
    ${GENERATED_OUTPUTS}/rules_db.ddl
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${GENERATED_OUTPUTS}/rules_db.ddl
  COMMENT "Compiling rules_db.ddl."
)

add_custom_target(generate_rules_db
  DEPENDS ${GENERATED_OUTPUTS}/gaia_rules_db.h
)

# main target
add_library(gaia_rules STATIC
  src/event_manager.cpp
  src/rule_thread_pool.cpp
  src/rule_context.cpp
  src/rule_checker.cpp
)

add_dependencies(gaia_rules generate_event_logs)
set_target_properties(gaia_rules PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(gaia_rules PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})
target_link_libraries(gaia_rules PRIVATE gaia_direct)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_rules PROPERTY POSITION_INDEPENDENT_CODE ON)

# unit tests
add_gtest(test_auto_tx "tests/test_auto_tx.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_semock")
add_gtest(test_event_manager "tests/test_event_manager.cpp;tests/event_manager_test_helpers.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia")
add_gtest(test_system_init "tests/test_system_init.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia")
add_gtest(test_component_init "tests/test_component_init.cpp;tests/event_manager_test_helpers.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_rules;gaia_direct;gaia_semock;gaia_catalog")
add_gtest(test_rule_context "tests/test_rule_context.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_rules")
add_gtest(test_rule_checker "tests/test_rule_checker.cpp;"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia")
add_gtest(test_rule_integration "tests/test_rule_integration.cpp"
  "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia" "generate_rules_db")
