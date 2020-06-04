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
  ${PROJECT_SOURCE_DIR}/inc
  ${GENERATED_OUTPUTS}
)
message(STATUS "GAIA_CATALOG_TOOL_INCLUDES=${GAIA_CATALOG_TOOL_INCLUDES}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
bison_target(parser
  src/parser.yy
  ${GENERATED_OUTPUTS}/parser.cpp)
flex_target(lexer
  src/lexer.ll
  ${GENERATED_OUTPUTS}/lexer.cpp)
add_flex_bison_dependency(lexer parser)

add_executable(gaiac
  src/main.cpp
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})
target_include_directories(gaiac PRIVATE ${GAIA_CATALOG_TOOL_INCLUDES})
