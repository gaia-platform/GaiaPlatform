#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(hackathon)

# This file assumes that the Gaia SDK has been installed into /opt/gaia.
# Useful for running in Docker container.
set(CMAKE_CXX_STANDARD 17)
set(GAIA "/opt/gaia")
set(LOCAL "/usr/local")
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}")
set(GAIA_INC "${GAIA}/include")

set(GAIA_INCUBATOR_DEMO_INCLUDES
  ${GAIA_INC}/common
  ${GAIA_INC}/direct_access
  ${GAIA_INC}/system
  ${GAIA_INC}/common
  ${GAIA_INC}/rules
  ${GAIA_INC}/db
  ${GAIA_INC}
  ${CMAKE_SOURCE_DIR}
  ${GENERATED_OUTPUTS}
)

set(GAIA_GENERATE "${GAIA}/bin/gaiac")
set(GAIA_TRANSLATE "${GAIA}/bin/gaiat")

set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/hack.ddl)
set(INCUBATOR_DDL_SOURCE ${PROJECT_SOURCE_DIR}/hack-comments.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/hack.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

# The source DDL file, hack-comments.ddl, is preprocessed here using cpp
# and sed. The generated hack.ddl is used by the rest of the build.
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_hack.h
  COMMAND echo "Creating ${GENERATED_OUTPUTS}/gaia_hack.h"
  COMMAND cpp ${INCUBATOR_DDL_SOURCE} > ${INCUBATOR_DDL}
  COMMAND sed -i "{s/^#.*$$//}" ${INCUBATOR_DDL}
  COMMAND pkill gaia_db_server
  COMMAND sleep 2
  COMMAND ${GAIA_GENERATE} -g ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_DDL_SOURCE}
  COMMENT "Compiling hack.ddl."
)

add_custom_target(generate_hack_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_hack.h
)
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/hack_rules.cpp
  COMMAND echo "${GAIA_TRANSLATE} ${INCUBATOR_RULESET} -output ${PROJECT_BINARY_DIR}/hack_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/9/include/"
  COMMAND ${GAIA_TRANSLATE} ${INCUBATOR_RULESET} -output ${PROJECT_BINARY_DIR}/hack_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/9/include/ -I /opt/gaia/include
  DEPENDS ${INCUBATOR_RULESET}
  COMMENT "Translating hack.ruleset."
)

add_custom_target(translate_hack_ruleset ALL
  DEPENDS generate_hack_headers
  DEPENDS ${PROJECT_BINARY_DIR}/hack_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(hack 
    main.cpp
    ${PROJECT_BINARY_DIR}/hack_rules.cpp
)

# The hackathon app uses a system configuration and logger configuration file.
configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${PROJECT_SOURCE_DIR}/gaia_log.conf" "${PROJECT_BINARY_DIR}/gaia_log.conf")

add_dependencies(hack translate_hack_ruleset)
set_target_properties(hack PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(hack PRIVATE ${GAIA_INCUBATOR_DEMO_INCLUDES})
target_link_libraries(hack PRIVATE gaia rt pthread)

if (EXISTS ${CMAKE_SOURCE_DIR}/var_list.in)
    configure_file(${CMAKE_SOURCE_DIR}/var_list.in ${CMAKE_BINARY_DIR}/var_list.txt)
endif()
