#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(gaiac)

set(GENERATED_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}")

set(GAIA_CATALOG_TOOL_INCLUDES
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GENERATED_OUTPUTS}
)
message(STATUS "GAIA_CATALOG_TOOL_INCLUDES=${GAIA_CATALOG_TOOL_INCLUDES}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
bison_target(parser
  src/parser.yy
  ${GENERATED_OUTPUTS}/parser.cpp
  DEFINES_FILE ${GENERATED_OUTPUTS}/parser.hpp)
flex_target(lexer
  src/lexer.ll
  ${GENERATED_OUTPUTS}/lexer.cpp
  DEFINES_FILE ${GENERATED_OUTPUTS}/lexer.h)
add_flex_bison_dependency(lexer parser)

add_executable(gaiac
  src/main.cpp
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})
target_include_directories(gaiac PRIVATE ${GAIA_CATALOG_TOOL_INCLUDES})
target_link_libraries(gaiac PRIVATE rt gaia gaia_catalog)
set_target_properties(gaiac PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaiac PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

set(TEST_SOURCES
  tests/test_ddl_parser.cpp
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})
message(STATUS "TEST_SOURCES=${TEST_SOURCES}")
add_gtest(test_ddl_scanner "${TEST_SOURCES}" "${GAIA_CATALOG_TOOL_INCLUDES}" "")
