#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(parser)

set(GAIA_PARSER_INCLUDES
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GAIA_PARSER_GENERATED_OUTPUTS}
)
message(STATUS "GAIA_PARSER_INCLUDES=${GAIA_PARSER_INCLUDES}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
bison_target(parser
  src/parser.yy
  ${GAIA_PARSER_GENERATED_OUTPUTS}/yy_parser.cpp
  DEFINES_FILE ${GAIA_PARSER_GENERATED_OUTPUTS}/yy_parser.hpp)
flex_target(lexer
  src/lexer.ll
  ${GAIA_PARSER_GENERATED_OUTPUTS}/yy_lexer.cpp
  DEFINES_FILE ${GAIA_PARSER_GENERATED_OUTPUTS}/yy_lexer.hpp)
add_flex_bison_dependency(lexer parser)


add_library(gaia_parser STATIC
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})
target_include_directories(gaia_parser PRIVATE ${GAIA_PARSER_INCLUDES})
target_link_libraries(gaia_parser PRIVATE rt gaia gaia_catalog)
set_target_properties(gaia_parser PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_parser PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

set(DDL_PARSER_TEST_SOURCES
  tests/test_ddl_parser.cpp
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})
add_gtest(test_ddl_parser "${DDL_PARSER_TEST_SOURCES}" "${GAIA_PARSER_INCLUDES}" "")
