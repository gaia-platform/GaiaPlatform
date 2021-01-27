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

set(GAIA_INC "${GAIA}/include")
set(GAIA_GENERATE "${GAIA}/bin/gaiac")
set(GAIA_TRANSLATE "${GAIA}/bin/gaiat")

set(HELLO_DDL ${PROJECT_SOURCE_DIR}/hello.ddl)
set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/hello.ruleset)

add_custom_command(
  COMMENT "Compiling hello.ddl..."
  OUTPUT ${PROJECT_BINARY_DIR}/gaia_hello.h
  COMMAND ${GAIA_GENERATE}
    -o ${PROJECT_BINARY_DIR}
    -g ${HELLO_DDL}
  DEPENDS ${HELLO_DDL}
)

add_custom_target(generate_hello_headers ALL
  DEPENDS ${PROJECT_BINARY_DIR}/gaia_hello.h
)

add_custom_command(
  COMMENT "Translating hello.ruleset..."
  OUTPUT ${PROJECT_BINARY_DIR}/hello_rules.cpp
  COMMAND ${GAIA_TRANSLATE}
    ${HELLO_RULESET}
    -output ${PROJECT_BINARY_DIR}/hello_rules.cpp
    --
    -I /usr/lib/clang/8/include/
    -I ${GAIA_INC}
    -I ${PROJECT_BINARY_DIR}
  DEPENDS ${HELLO_RULESET}
)

add_custom_target(translate_hello_ruleset ALL
  DEPENDS generate_hello_headers
  DEPENDS ${PROJECT_BINARY_DIR}/hello_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(hello
  hello.cpp
  ${PROJECT_BINARY_DIR}/hello_rules.cpp
)

add_dependencies(hello translate_hello_ruleset)
set_target_properties(hello PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
set_target_properties(hello PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
target_include_directories(hello PRIVATE ${GAIA_INC})
target_include_directories(hello PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(hello PRIVATE rt pthread gaia)
