#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(ping_pong)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -Wall -O3")

set(GAIA "/opt/gaia")

include(${GAIA}/cmake/gaia.cmake)
set(GAIA_INC "${GAIA}/include")

set(PING_PONG_DDL ${PROJECT_SOURCE_DIR}/ping_pong.ddl)
set(PING_PONG_RULESET ${PROJECT_SOURCE_DIR}/ping_pong.ruleset)

# --- Generate EDC from DDL---
process_schema(
  DDL_FILE ${PING_PONG_DDL}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR})

# -- Translate ruleset into CPP --
translate_ruleset(
  RULESET_FILE ${PING_PONG_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  DEPENDS generate_ping_pong_headers)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia.so)

add_executable(ping_pong
  ping_pong.cpp
  ${PROJECT_BINARY_DIR}/ping_pong_ruleset.cpp
)

add_dependencies(ping_pong translate_ping_pong_ruleset)
target_include_directories(ping_pong PRIVATE ${GAIA_INC})
target_include_directories(ping_pong PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(ping_pong PRIVATE rt pthread gaia)
