#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(prerequisites_schema)

set(PREREQUISITES_DDL "${PROJECT_SOURCE_DIR}/prerequisites.ddl")

string(RANDOM PREREQUISITES_INSTANCE_NAME)

#
# Generate stand alone prerequisites.
#
add_custom_command(
  COMMENT "Compiling prerequisites.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_prerequisites.h
  COMMAND ${GAIA_GENERATE}
    -t ${GAIA_PROD_BUILD}/db/core
    -o ${GAIA_GENERATED_SCHEMAS}
    -n ${PREREQUISITES_INSTANCE_NAME}
    -d prerequisites
    -g ${PREREQUISITES_DDL}
  DEPENDS gaiac
  DEPENDS ${PREREQUISITES_DDL}
)

add_custom_target(generate_prerequisites_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_prerequisites.h
)
