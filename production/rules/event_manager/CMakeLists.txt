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
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/internal/rules
  ${GAIA_RULES_INC}
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GENERATED_OUTPUTS}
)
message(STATUS "GAIA_EVENT_MANAGER_INCLUDES=${GAIA_EVENT_MANAGER_INCLUDES}")

# Generate the event log headers from the flatbuffer schema.
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp_opt(flatbuffers/event_log.fbs "--no-includes;--gen-compare;--gaia-type-initial-value;6" "${GENERATED_OUTPUTS}")
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_event_logs
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

gaia_compile_flatbuffers_schema_to_cpp_opt(flatbuffers/addr_book.fbs "--no-includes;--gen-compare;--gaia-type-initial-value;1000" "${GENERATED_OUTPUTS}")
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_addr_book
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

# main target
add_library(gaia_rules STATIC
  src/event_manager.cpp
  src/rule_thread_pool.cpp
  src/rule_context.cpp
)
add_dependencies(gaia_rules generate_event_logs)
set_target_properties(gaia_rules PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(gaia_rules PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})
target_link_libraries(gaia_rules PRIVATE gaia_semock)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_rules PROPERTY POSITION_INDEPENDENT_CODE ON)

# unit tests
add_gtest(test_auto_tx "tests/test_auto_tx.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_semock")
add_gtest(test_event_manager "tests/test_event_manager.cpp;tests/event_manager_test_helpers.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia")
add_gtest(test_system_init "tests/test_system_init.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia")
add_gtest(test_component_init "tests/test_component_init.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_rules;gaia_direct;gaia_semock")
add_gtest(test_rule_context "tests/test_rule_context.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia_rules")
add_gtest(test_rule_integration "tests/test_rule_integration.cpp" "${GAIA_EVENT_MANAGER_INCLUDES}" "rt;gaia" "generate_addr_book")
