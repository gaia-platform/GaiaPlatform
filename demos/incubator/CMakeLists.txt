#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(incubator_demo)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/incubator/generated")

set(GAIA_INCUBATOR_DEMO_INCLUDES
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/system
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/internal/rules
  ${GAIA_INC}/public/db
  ${GAIA_INC}/internal/db
  ${FLATBUFFERS_INC}
  ${GENERATED_OUTPUTS}
)

set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)
set(GAIA_TRANSLATE ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat)

set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/incubator.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/incubator.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_incubator.h
  COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server &
  COMMAND sleep 1
  COMMAND ${GAIA_GENERATE} -o ${GENERATED_OUTPUTS} -g ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_DDL}
  COMMENT "Compiled incubator.ddl."
)

add_custom_target(generate_incubator_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_incubator.h
)
add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/incubator_rules.cpp
  COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server &
  COMMAND sleep 1
  COMMAND ${GAIA_TRANSLATE} ${INCUBATOR_RULESET} -output ${PROJECT_SOURCE_DIR}/incubator_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/8/include/
  DEPENDS ${INCUBATOR_RULESET}
  COMMENT "Translating incubator.ruleset."
)

add_custom_target(translate_incubator_ruleset ALL
  DEPENDS generate_incubator_headers
  DEPENDS ${PROJECT_SOURCE_DIR}/incubator_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/sdk/libgaia.so)

add_executable(incubator 
    main.cpp
    incubator_rules.cpp
)

# The incubator demo uses a system configuration and logger configuration file.
configure_file("${PROJECT_SOURCE_DIR}/gaia_conf.toml" "${PROJECT_BINARY_DIR}/gaia_conf.toml")
configure_file("${PROJECT_SOURCE_DIR}/log_conf.toml" "${PROJECT_BINARY_DIR}/log_conf.toml")

add_dependencies(incubator translate_incubator_ruleset)
set_target_properties(incubator PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(incubator PRIVATE ${GAIA_INCUBATOR_DEMO_INCLUDES})
target_link_libraries(incubator PRIVATE rt pthread gaia)
