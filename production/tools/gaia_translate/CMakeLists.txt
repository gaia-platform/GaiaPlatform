cmake_minimum_required(VERSION 3.17)
enable_testing()
project(gaiat VERSION 0.0.0)

set(TRANSLATION_ENGINE_INCLUDES
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/clang/include
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/llvm/include
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_PROD_BUILD}/llvm/tools/clang/include
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_INTERNAL_INC}/catalog
  ${GAIA_REPO}/production/catalog/catalog_manager/inc
  ${GAIA_INC}
  ${GAIA_INTERNAL_INC}/common
  ${GAIA_INTERNAL_INC}/db
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PROD_BUILD}/catalog/parser/generated
  )

set(GAIAT_SRCS
  src/main.cpp
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
  gaia_se_client
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

set(INCUBATOR_DDL "${GAIA_REPO}/production/schemas/test/incubator/barn_storage.ddl")
set(INCUBATOR_RULESET "${GAIA_REPO}/production/tools/gaia_translate/tests/test.ruleset")

add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/test.cpp
  COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server --disable-persistence &
  COMMAND sleep 1
  COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${INCUBATOR_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${INCUBATOR_RULESET} -output ${GAIA_GENERATED_SCHEMAS}/test.cpp --
  COMMAND pkill -f -KILL gaia_se_server &
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_RULESET}
  COMMENT "Compiled test.ruleset."
)

add_custom_target(generate_incubator_rules ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/test.cpp
  # Add previous target to realize parallel builds.
  DEPENDS generate_incubator_headers
)

add_gtest(test_translation_engine
  "tests/test_translation_engine.cpp;${GAIA_GENERATED_SCHEMAS}/test.cpp"
  "${TRANSLATION_ENGINE_INCLUDES};${GAIA_GENERATED_SCHEMAS}"
  "rt;gaia_system;gaia_db_catalog_test"
  "generate_incubator_rules" ""
)

# Test declarative code mixed with cpp

set(MIXED_RULESET "${GAIA_REPO}/production/tools/gaia_translate/tests/test_mixed.ruleset")
set(MIXED_RULESET_OUT "${GAIA_GENERATED_SCHEMAS}/test_mixed.cpp")

add_custom_command(
  COMMENT "Compiling test_mixed.ruleset."
  OUTPUT ${MIXED_RULESET_OUT}
  COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server --disable-persistence &
  COMMAND sleep 1
  COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${INCUBATOR_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${MIXED_RULESET} -output ${MIXED_RULESET_OUT} --
    -I ${GAIA_INC}
    -I ${GAIA_GENERATED_SCHEMAS}
    -I ${FLATBUFFERS_INC}
    -I /usr/include/clang/10/include/
  COMMAND pkill -f -KILL gaia_se_server &
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS ${INCUBATOR_DDL}
  DEPENDS ${MIXED_RULESET}
)

add_custom_target(generate_mixed_rules ALL
  DEPENDS ${MIXED_RULESET_OUT}
  DEPENDS generate_incubator_rules
)

add_gtest(test_mixed_ruleset
  "tests/test_mixed_ruleset.cpp;${MIXED_RULESET_OUT}"
  "${TRANSLATION_ENGINE_INCLUDES};${GAIA_GENERATED_SCHEMAS}"
  "rt;gaia_system;gaia_db_catalog_test"
  "generate_mixed_rules" ""
)
