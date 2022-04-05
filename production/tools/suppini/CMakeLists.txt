enable_testing()
project(suppini VERSION 0.0.0)

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

link_directories(
  ${GAIA_PROD_BUILD}/lib
)

add_executable(suppini
  src/main.cpp
)

configure_gaia_target(suppini)
target_include_directories(suppini SYSTEM PRIVATE ${TRANSLATION_ENGINE_SYSTEM_INCLUDES})
target_include_directories(suppini PRIVATE ${TRANSLATION_ENGINE_PROJECT_INCLUDES})
add_dependencies(suppini gaia_db_server_exec)
target_link_libraries(suppini
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
  target_link_libraries(suppini PRIVATE gaia_stack_trace)
endif()


set(SUPPINI_CMD "${GAIA_PROD_BUILD}/tools/suppini/suppini")
set(TEST_FOLDER "${GAIA_REPO}/production/tools/suppini/tests")

set(TEST_INPUT "${TEST_FOLDER}/test.cpp")
set(TEST_OUTPUT "${TEST_FOLDER}/test_out.cpp")

add_custom_command(
  OUTPUT ${TEST_OUTPUT}
  COMMAND ${SUPPINI_CMD} ${TEST_INPUT} -output ${TEST_OUTPUT} --
  DEPENDS ${SUPPINI_CMD}
  DEPENDS ${TEST_INPUT}
)

# Smoke test for basic ruleset translation functionality
add_gtest(test_suppini
  "tests/test_suppini.cpp;${TEST_OUTPUT}"
  "${GAIA_INC}"
  "rt"
)
