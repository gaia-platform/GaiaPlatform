cmake_minimum_required(VERSION 3.17)
enable_testing()
project(gaiat VERSION 0.0.0)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

set(TRANSLATION_ENGINE_INCLUDES
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/clang/include
  ${GAIA_REPO}/third_party/production/TranslationEngineLLVM/llvm/include
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_PROD_BUILD}/llvm/tools/clang/include
  ${GAIA_PROD_BUILD}/llvm/include
  ${GAIA_INC}/internal/catalog
  ${GAIA_REPO}/production/catalog/catalog_manager/inc
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/db
  ${GAIA_INC}/public/db
  ${FLATBUFFERS_INC}
  ${GAIA_INC}/public/rules
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
  explain
)

set(INCUBATOR_DDL "${GAIA_REPO}/production/schemas/test/incubator/barn_storage.ddl")
set(INCUBATOR_RULESET "${GAIA_REPO}/production/tools/gaia_translate/tests/test.ruleset")


add_custom_command(
  OUTPUT  ${GAIA_GENERATED_SCHEMAS}/test.cpp
  COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server &
  COMMAND sleep 1
  COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac -destroy_db ${INCUBATOR_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${INCUBATOR_RULESET} -output ${GAIA_GENERATED_SCHEMAS}/test.cpp
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
  "rt;gaia_se_client;gaia_rules;gaia_direct;gaia_payload_types;gaia_common;gaia_catalog;gaia_parser;flatbuffers;explain"
  "generate_incubator_rules" ""
  "INCUBATOR_DDL_FILE=${GAIA_REPO}/production/schemas/test/incubator/barn_storage.ddl"
  )
