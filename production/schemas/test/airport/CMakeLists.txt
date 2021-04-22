#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(airport_schema)

set(AIRPORT_DDL "${PROJECT_SOURCE_DIR}/airport.ddl")

#
# Generate stand alone airport.
#
add_custom_command(
  COMMENT "Compiling airport.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_airport.h
  COMMAND ${GAIA_GENERATE} --destroy-db -t ${GAIA_PROD_BUILD}/db/core -o ${GAIA_GENERATED_SCHEMAS} -d airport -g ${AIRPORT_DDL}
  DEPENDS gaiac
  DEPENDS ${AIRPORT_DDL}
)

add_custom_target(generate_airport_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_airport.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_addr_book_headers
)
