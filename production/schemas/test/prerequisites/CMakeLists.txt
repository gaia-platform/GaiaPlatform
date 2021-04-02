#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(prerequisites_schema)

set(PREREQUISITES_DDL "${PROJECT_SOURCE_DIR}/prerequisites.ddl")

#
# Generate stand alone prerequisites.
#
add_custom_command(
  COMMENT "Compiling prerequisites.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_prerequisites.h
  COMMAND ${GAIA_GENERATE} --destroy-db -t ${GAIA_PROD_BUILD}/db/core -o ${GAIA_GENERATED_SCHEMAS} -g ${PREREQUISITES_DDL}
  DEPENDS gaiac
  DEPENDS ${PREREQUISITES_DDL}
)

add_custom_target(generate_prerequisites_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_prerequisites.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_incubator_headers
)
