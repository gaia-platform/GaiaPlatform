###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

enable_testing()
project(gaiat VERSION 0.0.0)

set(TRANSLATION_ENGINE_SYSTEM_INCLUDES
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/clang/include
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/llvm/include
)

set(TRANSLATION_ENGINE_PROJECT_INCLUDES
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_PROD_BUILD}/llvm/tools/clang/include
  ${GAIA_REPO}/production/catalog/catalog_manager/inc
  ${GAIA_INC}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PROD_BUILD}/catalog/parser/generated
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

set(GAIAT_SRCS
  src/diagnostics.cpp
  src/main.cpp
  src/table_navigation.cpp
)

link_directories(
  ${GAIA_PROD_BUILD}/lib
)

add_executable(gaiat ${GAIAT_SRCS})
configure_gaia_target(gaiat)
target_include_directories(gaiat SYSTEM PRIVATE ${TRANSLATION_ENGINE_SYSTEM_INCLUDES})
target_include_directories(gaiat PRIVATE ${TRANSLATION_ENGINE_PROJECT_INCLUDES})
add_dependencies(gaiat gaia_db_server_exec)
target_link_libraries(gaiat
  PRIVATE
  rt
  clangTooling
  gaia_db_client
  gaia_catalog
  gaia_direct
  gaia_payload_types
  gaia_system
  flatbuffers
  ${LIB_EXPLAIN}
  ${LIB_CAP}
  dl
)

if(ENABLE_STACKTRACE)
  target_link_libraries(gaiat PRIVATE gaia_stack_trace)
endif()

install(TARGETS gaiat DESTINATION ${CMAKE_INSTALL_BINDIR})

if("$CACHE{ENABLE_SDK_TESTS}")
  set(TEST_INCLUDES
    ${GAIA_INC}
    ${GAIA_REPO}/production/tools/gaia_translate/tests
    ${GAIA_REPO}/production/rules/event_manager/inc
  )

  set(DDL_PREFIX "${GAIA_REPO}/production/schemas/test")
  set(RULESET_PREFIX "${GAIA_REPO}/production/tools/gaia_translate/tests")
  set(RULES_TEST_HELPERS "${GAIA_REPO}/production/rules/event_manager/tests/rules_test_helpers.cpp")

  # The rules perf test needs a logger configuration to ensure that we write out
  # the rule stats.
  configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_translation.ruleset
    DAC_LIBRARY dac_barn_storage
  )

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_mixed.ruleset
    DAC_LIBRARY dac_barn_storage
  )

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_serial.ruleset
    CLANG_PARAMS -I${CMAKE_CURRENT_SOURCE_DIR}/tests
    DAC_LIBRARY dac_barn_storage
  )
  target_include_directories(test_serial_ruleset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_rulesets.ruleset
    DAC_LIBRARY dac_prerequisites
  )
  target_include_directories(test_rulesets_ruleset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_amr_swarm.ruleset
    DAC_LIBRARY dac_amr_swarm
  )
  target_include_directories(test_amr_swarm_ruleset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_metarule3.ruleset
    DAC_LIBRARY dac_prerequisites
  )
  target_include_directories(test_metarule3_ruleset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/test_array.ruleset
    DAC_LIBRARY dac_addr_book
  )
  target_include_directories(test_array_ruleset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  translate_ruleset_internal(
    RULESET_FILE ${RULESET_PREFIX}/perf_rules_basic.ruleset
    DAC_LIBRARY dac_perf_rules
  )
  target_include_directories(perf_rules_basic_ruleset PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  # Smoke test for basic ruleset translation functionality
  add_gtest(test_translation_engine
    "tests/test_translation_engine.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_translation_ruleset")

  # Test declarative code mixed with cpp
  add_gtest(test_mixed
    "tests/test_mixed_ruleset.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_mixed_ruleset")

  # Test serial_group attribute
  add_gtest(test_serial
    "tests/test_serial.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_serial_ruleset")

  # Test Preview extensions to Declarative Language
  add_gtest(test_tags_ruleset
    "tests/test_tags.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_rulesets_ruleset")

  add_gtest(test_queries_ruleset
    "tests/test_queries.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_rulesets_ruleset")

  add_gtest(test_insert_delete_ruleset
    "tests/test_insert_delete.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_rulesets_ruleset")

  add_gtest(test_connect_disconnect_ruleset
    "tests/test_connect_disconnect.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_rulesets_ruleset")

  add_gtest(test_amr_swarm
    "tests/test_amr_swarm.cpp;tests/bot_commands.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_amr_swarm_ruleset")

  add_gtest(test_metarule3
    "tests/test_metarule3.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_metarule3_ruleset")

  add_gtest(test_array
    "tests/test_array.cpp;${RULES_TEST_HELPERS}"
    "${TEST_INCLUDES}"
    "rt;gaia_system;gaia_db_catalog_test;test_array_ruleset")

  add_gtest(perf_rules_basic
    "tests/perf_rules_basic.cpp;${RULES_TEST_HELPERS};${GAIA_REPO}/production/common/src/debug_logger.cpp"
    "${TEST_INCLUDES};${GAIA_REPO}/production/benchmarks/inc"
    "rt;gaia_system;gaia_db_catalog_test;perf_rules_basic_ruleset")

endif()
