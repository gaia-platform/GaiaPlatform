#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(campus)

set(CMAKE_CXX_STANDARD 17)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

set(GAIA "/opt/gaia")
set(LOCAL "/usr/local")

include(${GAIA}/cmake/gaia.cmake)

set(GAIA_INC "${GAIA}/include")

set(CAMPUS_DDL ${PROJECT_SOURCE_DIR}/campus.ddl)
set(CAMPUS_RULESET ${PROJECT_SOURCE_DIR}/campus.ruleset)

# --- Generate EDC from DDL---
process_schema(
  DDL_FILE ${CAMPUS_DDL}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR})

# -- Translate ruleset into CPP --
translate_ruleset(
  RULESET_FILE ${CAMPUS_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  DEPENDS generate_campus_headers)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(campus
  campus.cpp
  ${PROJECT_BINARY_DIR}/campus_ruleset.cpp
)

add_dependencies(campus translate_campus_ruleset)
set_target_properties(campus PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(campus PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_include_directories(campus PRIVATE ${GAIA_INC})
target_include_directories(campus PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(campus PRIVATE rt pthread gaia)
