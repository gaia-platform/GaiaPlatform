###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

# --- Generate DAC from DDL---
process_schema(
  DDL_FILE ${PING_PONG_DDL}
  DATABASE_NAME ping_pong
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  GAIAC_CMD ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac
)

# -- Translate ruleset into CPP --
translate_ruleset(
  RULESET_FILE ${PING_PONG_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  CLANG_PARAMS -I ${GAIA_INC} -I ${FLATBUFFERS_INC}
  DEPENDS generate_ping_pong_headers gaiat
  GAIAT_CMD ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat
)

add_executable(ping_pong
  ping_pong.cpp
  ${PROJECT_BINARY_DIR}/ping_pong_ruleset.cpp
)

add_dependencies(ping_pong translate_ping_pong_ruleset)
configure_gaia_target(ping_pong)
target_include_directories(ping_pong PRIVATE
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR})
target_link_libraries(ping_pong PRIVATE pthread gaia)
