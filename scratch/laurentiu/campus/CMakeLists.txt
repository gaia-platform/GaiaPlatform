#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(campus)

set(CMAKE_CXX_STANDARD 17)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")

set(GAIA "/opt/gaia")
set(LOCAL "/usr/local")

set(GAIA_INC "${GAIA}/include")
set(GAIAC "${GAIA}/bin/gaiac")
set(GAIAT "${GAIA}/bin/gaiat")

set(CAMPUS_DDL ${PROJECT_SOURCE_DIR}/campus.ddl)
set(CAMPUS_RULESET ${PROJECT_SOURCE_DIR}/campus.ruleset)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/gaia_campus.h
  COMMAND ${GAIAC}
    -o ${PROJECT_BINARY_DIR}
    -g ${CAMPUS_DDL}
  DEPENDS ${CAMPUS_DDL}
  COMMENT "Processed campus.ddl."
)

add_custom_target(process_campus_ddl ALL
  DEPENDS ${PROJECT_BINARY_DIR}/gaia_campus.h
)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/campus_rules.cpp
  COMMAND ${GAIAT}
    ${CAMPUS_RULESET}
    -output ${PROJECT_BINARY_DIR}/campus_rules.cpp
    --
    -I /usr/lib/clang/8/include/
    -I ${GAIA_INC}
    -I ${PROJECT_BINARY_DIR}
  DEPENDS ${CAMPUS_RULESET}
  COMMENT "Processed campus.ruleset."
)

add_custom_target(process_campus_ruleset ALL
  DEPENDS process_campus_ddl
  DEPENDS ${PROJECT_BINARY_DIR}/campus_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(campus
  campus.cpp
  ${PROJECT_BINARY_DIR}/campus_rules.cpp
)

add_dependencies(campus process_campus_ruleset)
set_target_properties(campus PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(campus PRIVATE ${GAIA_INC})
target_include_directories(campus PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(campus PRIVATE rt pthread gaia)
