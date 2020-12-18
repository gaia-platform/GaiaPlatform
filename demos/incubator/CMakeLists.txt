#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# {SDK-SETUP:CMAKE} - Do not remove this line

# Set the project name.
project(incubator)

# {SDK-SETUP:CXX_STANDARD} - Do not remove this line
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")

# Enable AddressSanitizer in debug mode.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address")
endif()

# {SDK-SETUP:VARS} - Do not remove this line
set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/incubator.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/incubator.ruleset)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/gaia_incubator.h
  COMMAND ${GAIA_GENERATE} -o ${PROJECT_BINARY_DIR} -g ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_DDL}
  COMMENT "Compiled incubator.ddl."
)
add_custom_target(generate_incubator_headers ALL
  DEPENDS ${PROJECT_BINARY_DIR}/gaia_incubator.h
)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/incubator_rules.cpp
  COMMAND ${GAIA_TRANSLATE} ${INCUBATOR_RULESET} -output ${PROJECT_BINARY_DIR}/incubator_rules.cpp -- -I /usr/lib/clang/10/include/
  DEPENDS ${INCUBATOR_RULESET}
  COMMENT "Translating incubator.ruleset."
)
add_custom_target(translate_incubator_ruleset ALL
  DEPENDS generate_incubator_headers
  DEPENDS ${PROJECT_BINARY_DIR}/incubator_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_LIB})

add_executable(incubator
    incubator.cpp
    ${PROJECT_BINARY_DIR}/incubator_rules.cpp
)

# The incubator demo uses a system configuration and logger configuration file.
configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${PROJECT_SOURCE_DIR}/gaia_log.conf" "${PROJECT_BINARY_DIR}/gaia_log.conf")

add_dependencies(incubator translate_incubator_ruleset)
set_target_properties(incubator PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(incubator PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_include_directories(incubator PRIVATE ${GAIA_INC} ${PROJECT_BINARY_DIR})
target_link_libraries(incubator PRIVATE gaia rt pthread)
