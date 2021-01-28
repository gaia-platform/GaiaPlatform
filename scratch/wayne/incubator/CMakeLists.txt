#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(incubator)

set(CMAKE_CXX_STANDARD 17)
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")

# Enable AddressSanitizer in debug mode.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address")
endif()

set(GAIA_INC
  "/home/wayne/src/github.com/gaia-platform/GaiaPlatform/production/inc"
  "/home/wayne/src/github.com/gaia-platform/GaiaPlatform/third_party/production/flatbuffers/include"
)
set(GAIA_LIB "/home/wayne/src/github.com/gaia-platform/GaiaPlatform/build/master/sdk/libgaia.so")
set(GAIA_GENERATE "/home/wayne/src/github.com/gaia-platform/GaiaPlatform/build/master/catalog/gaiac/gaiac")
set(GAIA_TRANSLATE "/home/wayne/src/github.com/gaia-platform/GaiaPlatform/build/master/tools/gaia_translate/gaiat")
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

# The incubator demo uses a system configuration and logger configuration file.
configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${PROJECT_SOURCE_DIR}/gaia_log.conf" "${PROJECT_BINARY_DIR}/gaia_log.conf")

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_LIB})

add_executable(incubator
  incubator.cpp
  ${PROJECT_BINARY_DIR}/incubator_rules.cpp
)

add_executable(sensor sensor.cpp
  ${PROJECT_BINARY_DIR}/incubator_rules.cpp
)

add_dependencies(incubator translate_incubator_ruleset)
add_dependencies(sensor translate_incubator_ruleset)
set_target_properties(incubator PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(sensor PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(incubator PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
set_target_properties(sensor PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_include_directories(incubator PRIVATE ${GAIA_INC} ${PROJECT_BINARY_DIR})
target_include_directories(sensor PRIVATE ${GAIA_INC} ${PROJECT_BINARY_DIR})
target_link_libraries(incubator PRIVATE gaia rt pthread)
target_link_libraries(sensor PRIVATE gaia rt pthread)
