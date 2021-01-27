#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(gaia_generate)

set(GAIA_GENERATE_INCLUDES
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/db/core/src
  ${GAIA_PROD_BUILD}/catalog/parser/generated
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_REPO}/production/db/core/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${GAIA_INC}
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_GENERATE_INCLUDES=${GAIA_GENERATE_INCLUDES}")

set(CMAKE_CXX_CLANG_TIDY clang-tidy)

add_executable(gaia_generate src/main.cpp)
target_include_directories(gaia_generate PRIVATE ${GAIA_GENERATE_INCLUDES})
target_link_libraries(gaia_generate PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaia_generate PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_generate PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

set(GAIA_TEST_GENERATE_PRIVATE_INCLUDES
  ${GAIA_GENERATE_INCLUDES}
  ${GAIA_GENERATED_SCHEMAS}
)

add_gtest(test_gaia_generate
  "tests/test_gaia_generate.cpp"
  "${GAIA_TEST_GENERATE_PRIVATE_INCLUDES}"
  "rt;gaia_system;gaia_db_catalog_test"
  "generate_airport_headers" "")
