###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(parser)

file(MAKE_DIRECTORY ${GAIA_PARSER_GENERATED})

set(GAIA_PARSER_INCLUDES
  ${GAIA_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GAIA_PARSER_GENERATED}
)

message(VERBOSE "GAIA_PARSER_INCLUDES=${GAIA_PARSER_INCLUDES}")

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
bison_target(parser
  src/parser.yy
  ${GAIA_PARSER_GENERATED}/yy_parser.cpp
  DEFINES_FILE ${GAIA_PARSER_GENERATED}/yy_parser.hpp)
flex_target(lexer
  src/lexer.ll
  ${GAIA_PARSER_GENERATED}/yy_lexer.cpp)
add_flex_bison_dependency(lexer parser)

add_library(gaia_parser STATIC
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})

configure_gaia_target(gaia_parser)
target_include_directories(gaia_parser PRIVATE ${GAIA_PARSER_INCLUDES})
target_link_libraries(gaia_parser PRIVATE gaia_catalog)

set(DDL_PARSER_TEST_SOURCES
  tests/test_ddl_parser.cpp
  ${FLEX_lexer_OUTPUTS}
  ${BISON_parser_OUTPUTS})
add_gtest(test_ddl_parser "${DDL_PARSER_TEST_SOURCES}" "${GAIA_PARSER_INCLUDES}" "")
