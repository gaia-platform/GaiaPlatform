#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(hello)

set(CMAKE_CXX_STANDARD 17)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

set(GAIA "/opt/gaia")
set(LOCAL "/usr/local")

include(${GAIA}/cmake/gaia.cmake)

set(GAIA_INC "${GAIA}/include")

set(HELLO_DDL ${PROJECT_SOURCE_DIR}/hello.ddl)
set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/hello.ruleset)

# --- Generate EDC from DDL---
generate_schema_headers(
  DDL_FILE ${HELLO_DDL}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR})

# -- Translate ruleset into CPP --
translate_ruleset_code(
  RULESET_FILE ${HELLO_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  DEPENDS generate_hello_headers)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(hello
  hello.cpp
  ${PROJECT_BINARY_DIR}/hello_ruleset.cpp
)

add_dependencies(hello translate_hello_code)
set_target_properties(hello PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
set_target_properties(hello PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
target_include_directories(hello PRIVATE ${GAIA_INC})
target_include_directories(hello PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(hello PRIVATE rt pthread gaia)
