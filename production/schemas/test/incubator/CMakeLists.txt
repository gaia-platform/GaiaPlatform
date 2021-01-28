#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(incubator)

set(INCUBATOR_DDL "${PROJECT_SOURCE_DIR}/barn_storage.ddl")

#
# Generate stand alone incubator.
#
add_custom_command(
  COMMENT "Compiling incubator.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_barn_storage.h
  COMMAND ${GAIA_GENERATE} --destroy-db -t ${GAIA_PROD_BUILD}/db/core -o ${GAIA_GENERATED_SCHEMAS} -g ${INCUBATOR_DDL}
  DEPENDS gaiac
  DEPENDS ${INCUBATOR_DDL}
)

add_custom_target(generate_incubator_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_barn_storage.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_airport_headers
)
