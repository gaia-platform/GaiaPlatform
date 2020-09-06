#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(incubator)

set(INCUBATOR_DDL "${PROJECT_SOURCE_DIR}/barn_storage.ddl")
set(INCUBATOR_RULESET "${PROJECT_SOURCE_DIR}/test.ruleset")

#
# Generate stand alone airport.
#
add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_barn_storage.h
  COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server &
  COMMAND sleep 1
  COMMAND ${GAIA_GENERATE} -o ${GAIA_GENERATED_SCHEMAS} -g ${INCUBATOR_DDL}
  COMMAND ${GAIA_PROD_BUILD}/tools/gaia_TranslationEngine/gaiat ${INCUBATOR_RULESET} -output ${GAIA_GENERATED_SCHEMAS}/test.cpp
  COMMAND pkill -f -KILL gaia_se_server &
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_RULESET}
  COMMENT "Compiled incubator.ddl."
)

add_custom_target(generate_incubator_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_barn_storage.h
  # Add previous target to realize parallel builds.
  DEPENDS generate_airport_headers
)

