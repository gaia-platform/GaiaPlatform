#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(airport)

set(AIRPORT_DDL "${PROJECT_SOURCE_DIR}/airport.ddl")

#
# Generate stand alone airport.
#
add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_airport.h
  COMMAND ${GAIA_GENERATE} --destroy-db -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GAIA_GENERATED_SCHEMAS} -g ${AIRPORT_DDL}
  DEPENDS gaiac
  DEPENDS ${AIRPORT_DDL}
  COMMENT "Compiled airport.ddl."
)

add_custom_target(generate_airport_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_airport.h
  # Add previous target to rialize parallel builds.
  DEPENDS generate_addr_book_headers
)
