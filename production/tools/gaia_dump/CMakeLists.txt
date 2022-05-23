###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(gaia_dump)

set(GAIA_DUMP_INCLUDES
  ${GAIA_REPO}/production/db/core/src
  ${GAIA_PROD_BUILD}/catalog/parser/generated
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_REPO}/production/db/inc/core
  ${GAIA_INC}
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_DUMP_INCLUDES=${GAIA_DUMP_INCLUDES}")

add_executable(gaia_dump src/main.cpp src/gaia_dump.cpp)
configure_gaia_target(gaia_dump)
target_include_directories(gaia_dump PRIVATE ${GAIA_DUMP_INCLUDES})
target_link_libraries(gaia_dump PRIVATE rt gaia_catalog gaia_parser gaia_db_client Threads::Threads)

add_gtest(test_gaia_dump
  "tests/test_gaia_dump.cpp;src/gaia_dump.cpp"
  "${GAIA_DUMP_INCLUDES}"
  "rt;gaia_system;gaia_catalog;gaia_parser;dac_airport")
