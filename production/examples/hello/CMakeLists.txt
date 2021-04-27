#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(hello)

set(CMAKE_CXX_STANDARD 17)

configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

set(HELLO_DDL ${PROJECT_SOURCE_DIR}/hello.ddl)
set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/hello.ruleset)

# --- Generate EDC from DDL---
add_custom_command(
    COMMENT "Compiling hello.ddl..."
    OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_hello.h
    COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac -o ${PROJECT_BINARY_DIR} -d hello -g ${HELLO_DDL}
    DEPENDS gaiac
    DEPENDS ${INCUBATOR_DDL}
)

add_custom_target(generate_hello_headers ALL
    DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_hello.h
)

# -- Translate ruleset into CPP --
translate_ruleset(
  RULESET_FILE ${HELLO_RULESET}
  OUTPUT_FOLDER ${PROJECT_BINARY_DIR}
  CLANG_PARAMS -I ${GAIA_INC} -I ${FLATBUFFERS_INC}
  DEPENDS generate_hello_headers gaiat
  GAIAT_CMD ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat
)

add_executable(hello
    hello.cpp
    ${PROJECT_BINARY_DIR}/hello_ruleset.cpp
)

add_dependencies(hello translate_hello_ruleset)
set_target_properties(hello PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(hello PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
target_include_directories(hello PRIVATE ${GAIA_INC})
target_include_directories(hello PRIVATE ${PROJECT_BINARY_DIR})
target_include_directories(hello PRIVATE ${FLATBUFFERS_INC})
#target_include_directories(hello PRIVATE /home/simone/repos/spdlog/include)
target_link_libraries(hello PRIVATE gaia rt pthread)
