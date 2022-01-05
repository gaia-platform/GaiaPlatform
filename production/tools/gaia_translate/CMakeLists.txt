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

  # Smoke test for basic ruleset translation functionality
  add_gaia_sdk_gtest(
    TARGET_NAME "test_translation_engine"
    DDL_FILE "${DDL_PREFIX}/incubator/incubator.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_translation.ruleset"
    DATABASE_NAME "barn_storage"
    TARGET_SOURCES "tests/test_translation_engine.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  # Test declarative code mixed with cpp
  add_gaia_sdk_gtest(
    TARGET_NAME "test_mixed_ruleset"
    DDL_FILE "${DDL_PREFIX}/incubator/incubator.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_mixed.ruleset"
    DATABASE_NAME "barn_storage"
    PREVIOUS_TARGET_NAME "test_translation_engine"
    TARGET_SOURCES "tests/test_mixed_ruleset.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  # Test serial_group attribute
  add_gaia_sdk_gtest(
    TARGET_NAME "test_serial"
    DDL_FILE "${DDL_PREFIX}/incubator/incubator.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_serial.ruleset"
    DATABASE_NAME "barn_storage"
    PREVIOUS_TARGET_NAME "test_mixed_ruleset"
    TARGET_SOURCES "tests/test_serial.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  # Test Preview extensions to Declarative Language
  add_gaia_sdk_gtest(
    TARGET_NAME "test_tags_ruleset"
    DDL_FILE "${DDL_PREFIX}/prerequisites/prerequisites.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_rulesets.ruleset"
    DATABASE_NAME "prerequisites"
    PREVIOUS_TARGET_NAME "test_serial"
    TARGET_SOURCES "tests/test_tags.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  add_gaia_sdk_gtest(
    TARGET_NAME "test_queries_ruleset"
    DDL_FILE "${DDL_PREFIX}/prerequisites/prerequisites.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_rulesets.ruleset"
    DATABASE_NAME "prerequisites"
    PREVIOUS_TARGET_NAME "test_tags_ruleset"
    TARGET_SOURCES "tests/test_queries.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  add_gaia_sdk_gtest(
    TARGET_NAME "test_insert_delete_ruleset"
    DDL_FILE "${DDL_PREFIX}/prerequisites/prerequisites.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_rulesets.ruleset"
    DATABASE_NAME "prerequisites"
    PREVIOUS_TARGET_NAME "test_queries_ruleset"
    TARGET_SOURCES "tests/test_insert_delete.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  add_gaia_sdk_gtest(
    TARGET_NAME "test_connect_disconnect_ruleset"
    DDL_FILE "${DDL_PREFIX}/prerequisites/prerequisites.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_rulesets.ruleset"
    DATABASE_NAME "prerequisites"
    PREVIOUS_TARGET_NAME "test_insert_delete_ruleset"
    TARGET_SOURCES "tests/test_connect_disconnect.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  add_gaia_sdk_gtest(
    TARGET_NAME "test_amr_swarm_ruleset"
    DDL_FILE "${DDL_PREFIX}/amr_swarm/amr_swarm.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_amr_swarm.ruleset"
    DATABASE_NAME "amr_swarm"
    PREVIOUS_TARGET_NAME "test_connect_disconnect_ruleset"
    TARGET_SOURCES "tests/test_amr_swarm.cpp;tests/bot_commands.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )

  add_gaia_sdk_gtest(
    TARGET_NAME "test_meta_rule_3_ruleset"
    DDL_FILE "${DDL_PREFIX}/prerequisites/prerequisites.ddl"
    RULESET_FILE "${RULESET_PREFIX}/test_metarule3.ruleset"
    DATABASE_NAME "prerequisites"
    PREVIOUS_TARGET_NAME "test_amr_swarm_ruleset"
    TARGET_SOURCES "tests/test_metarule3.cpp;${RULES_TEST_HELPERS}"
    TARGET_INCLUDES "${TEST_INCLUDES}"
  )
endif()
