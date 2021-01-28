#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(ping_pong)

set(CMAKE_CXX_STANDARD 17)

set(PING_PONG_DDL ${PROJECT_SOURCE_DIR}/ping_pong.ddl)
set(PING_PONG_RULESET ${PROJECT_SOURCE_DIR}/ping_pong.ruleset)

configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

# --- Generate EDC from DDL---
process_schema(
  DDL_FILE ${PING_PONG_DDL}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR})

# -- Translate ruleset into CPP --
translate_ruleset(
  RULESET_FILE ${PING_PONG_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  DEPENDS generate_ping_pong_headers)

add_executable(ping_pong
  ping_pong.cpp
  ${PROJECT_BINARY_DIR}/ping_pong_ruleset.cpp
)

add_dependencies(ping_pong translate_ping_pong_ruleset)
set_target_properties(ping_pong PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(ping_pong PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_include_directories(ping_pong PRIVATE
  ${GAIA_INC}
  ${SPDLOG_INC}
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR})
target_link_libraries(ping_pong PRIVATE pthread gaia)
