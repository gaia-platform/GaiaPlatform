#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(event_planner)

set(CMAKE_CXX_STANDARD 17)
set(GAIA "/opt/gaia")
set(LOCAL "/usr/local")
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/generated")
set(GAIA_INC "${GAIA}/include")

set(GAIA_INCLUDES
  ${GAIA_INC}
  ${GENERATED_OUTPUTS}
  .
)

set(GAIA_GENERATE "${GAIA}/bin/gaiac")
set(GAIA_TRANSLATE "${GAIA}/bin/gaiat")

set(GAIA_U_DDL ${PROJECT_SOURCE_DIR}/gaia_u.ddl)
set(GAIA_U_RULESET ${PROJECT_SOURCE_DIR}/gaia_u.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_u_generated.h
  OUTPUT ${GENERATED_OUTPUTS}/gaia_gaia_u.h
  COMMAND ${GAIA_GENERATE} -o ${GENERATED_OUTPUTS} -g ${GAIA_U_DDL}
  DEPENDS ${GAIA_U_DDL}
  COMMENT "Compiled gaia_u.ddl."
)

add_custom_target(generate_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_u_generated.h
  DEPENDS ${GENERATED_OUTPUTS}/gaia_gaia_u.h
)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/gaia_u_rules.cpp
  COMMAND ${GAIA_TRANSLATE} ${GAIA_U_RULESET} -output ${PROJECT_BINARY_DIR}/gaia_u_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/9/include/ -I ${GENERATED_OUTPUTS} -I ${GAIA_INC}
  DEPENDS ${GAIA_U_RULESET}
  COMMENT "Translating ruleset."
)

add_custom_target(translate_ruleset ALL
  DEPENDS generate_headers
  DEPENDS ${PROJECT_BINARY_DIR}/gaia_u_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(planner
    main.cpp
    ${PROJECT_BINARY_DIR}/gaia_u_rules.cpp
)

# The incubator demo uses a system configuration and logger configuration file.
configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${PROJECT_SOURCE_DIR}/gaia_log.conf" "${PROJECT_BINARY_DIR}/gaia_log.conf")

add_dependencies(planner translate_ruleset)
set_target_properties(planner PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(planner PRIVATE ${GAIA_INCLUDES})
target_link_libraries(planner PRIVATE rt pthread gaia)

add_executable(loader
  data/main.cpp
  data/utils.cpp
  data/loader.cpp
)

add_dependencies(loader generate_headers)
target_include_directories(loader PRIVATE ${GAIA_INCLUDES})
set_target_properties(loader PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(loader PRIVATE rt pthread gaia)
