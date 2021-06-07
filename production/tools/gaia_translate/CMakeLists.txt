enable_testing()
project(gaiat VERSION 0.0.0)

set(TRANSLATION_ENGINE_INCLUDES
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/clang/include
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/llvm/include
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_PROD_BUILD}/llvm/tools/clang/include
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_REPO}/production/catalog/catalog_manager/inc
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PROD_BUILD}/catalog/parser/generated
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

set(GAIAT_SRCS
  src/main.cpp
  src/table_navigation.cpp
)

link_directories(
  ${GAIA_PROD_BUILD}/lib
)

add_executable(gaiat ${GAIAT_SRCS})
target_include_directories(gaiat PRIVATE ${TRANSLATION_ENGINE_INCLUDES})
set_target_properties(gaiat PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaiat PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
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

install(TARGETS gaiat DESTINATION ${CMAKE_INSTALL_BINDIR})

set(TEST_INCLUDES
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/tools/gaia_translate/tests
)

set(INCUBATOR_DDL "${GAIA_REPO}/production/schemas/test/incubator/incubator.ddl")
set(TRANSLATION_RULESET "${GAIA_REPO}/production/tools/gaia_translate/tests/test.ruleset")
set(TRANSLATION_RULESET_OUT "${GAIA_GENERATED_CODE}/test_translation.cpp")

add_custom_command(
  COMMENT "Compiling test.ruleset..."
  OUTPUT ${TRANSLATION_RULESET_OUT}
  COMMAND ${GAIA_PROD_BUILD}/db/core/gaia_db_server --disable-persistence &
  COMMAND sleep 1
  COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${INCUBATOR_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${TRANSLATION_RULESET} -output ${TRANSLATION_RULESET_OUT} --
      -I ${GAIA_INC}
      -I ${GAIA_SPDLOG_INC}
      -I /usr/include/clang/10/include/
      -std=c++${CMAKE_CXX_STANDARD}
  COMMAND pkill -f -KILL gaia_db_server &
  # In some contexts, the next attempt to start gaia_db_server precedes this kill, leading
  # to a build failure. A short sleep is currently fixing that, but may not be the
  # correct long-term solution.
  COMMAND sleep 1
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS ${INCUBATOR_DDL}
  DEPENDS ${TRANSLATION_RULESET}
)

add_custom_target(generate_translation_rules ALL
  DEPENDS ${TRANSLATION_RULESET_OUT}
)

add_gtest(test_translation_engine
  "tests/test_translation_engine.cpp;tests/test_helpers.cpp;${TRANSLATION_RULESET_OUT}"
  "${TEST_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test;edc_barn_storage"
  "generate_translation_rules" ""
)

# Test declarative code mixed with cpp

set(MIXED_RULESET "${GAIA_REPO}/production/tools/gaia_translate/tests/test_mixed.ruleset")
set(MIXED_RULESET_OUT "${GAIA_GENERATED_CODE}/test_mixed.cpp")

add_custom_command(
  COMMENT "Compiling test_mixed.ruleset..."
  OUTPUT ${MIXED_RULESET_OUT}
  COMMAND ${GAIA_PROD_BUILD}/db/core/gaia_db_server --disable-persistence &
  COMMAND sleep 1
  COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${INCUBATOR_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${MIXED_RULESET} -output ${MIXED_RULESET_OUT} --
    -I ${GAIA_INC}
    -I ${GAIA_GENERATED_CODE}/barn_storage
    -I ${FLATBUFFERS_INC}
    -I /usr/include/clang/10/include/
    -std=c++${CMAKE_CXX_STANDARD}
  COMMAND pkill -f -KILL gaia_db_server &
  # In some contexts, the next attempt to start gaia_db_server precedes this kill, leading
  # to a build failure. A short sleep is currently fixing that, but may not be the
  # correct long-term solution.
  COMMAND sleep 1
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS ${INCUBATOR_DDL}
  DEPENDS ${MIXED_RULESET}
)

add_custom_target(generate_mixed_rules ALL
  DEPENDS ${MIXED_RULESET_OUT}
  DEPENDS edc_barn_storage
  DEPENDS test_translation_engine
)

add_gtest(test_mixed_ruleset
  "tests/test_mixed_ruleset.cpp;${MIXED_RULESET_OUT}"
  "${TEST_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test;edc_barn_storage"
  "generate_mixed_rules" ""
)

# Test Preview extensions to Declarative Language

set(PREREQUISITE_DDL "${GAIA_REPO}/production/schemas/test/prerequisites/prerequisites.ddl")
set(RULESETS "${GAIA_REPO}/production/tools/gaia_translate/tests/test_rulesets.ruleset")
set(RULESETS_OUT "${GAIA_GENERATED_CODE}/test_rulesets.cpp")

add_custom_command(
  COMMENT "Compiling test_rulesets.ruleset..."
  OUTPUT ${RULESETS_OUT}
  COMMAND ${GAIA_PROD_BUILD}/db/core/gaia_db_server --disable-persistence &
  COMMAND sleep 1
  # message(WARNING, "${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${PREREQUISITE_DDL}")
  COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${PREREQUISITE_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${RULESETS} -output ${RULESETS_OUT} --
    -I ${GAIA_INC}
    -I ${FLATBUFFERS_INC}
    -I /usr/include/clang/10/include/
    -std=c++${CMAKE_CXX_STANDARD}
  COMMAND pkill -f -KILL gaia_db_server &
  # In some contexts, the next attempt to start gaia_db_server precedes this kill, leading
  # to a build failure. A short sleep is currently fixing that, but may not be the
  # correct long-term solution.
  COMMAND sleep 1
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS ${PREREQUISITE_DDL}
  DEPENDS ${RULESETS}
)

add_custom_target(generate_rulesets_rules ALL
  DEPENDS ${RULESETS_OUT}
  DEPENDS edc_prerequisites
  DEPENDS test_mixed_ruleset
)

add_gtest(test_tags_ruleset
  "tests/test_tags.cpp;tests/test_helpers.cpp;${RULESETS_OUT}"
  "${TEST_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test;edc_prerequisites"
  "generate_rulesets_rules" ""
)

add_gtest(test_queries_ruleset
  "tests/test_queries.cpp;tests/test_helpers.cpp;${RULESETS_OUT}"
  "${TEST_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test;edc_prerequisites"
  "generate_rulesets_rules" ""
)
