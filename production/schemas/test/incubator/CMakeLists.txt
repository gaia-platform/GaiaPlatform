#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(incubator_schema)

set(INCUBATOR_DDL "${PROJECT_SOURCE_DIR}/incubator.ddl")

string(RANDOM INCUBATOR_INSTANCE_NAME)

#
# Generate stand alone incubator.
#
add_custom_command(
  COMMENT "Compiling incubator.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_barn_storage.h
  COMMAND ${GAIA_GENERATE}
    -t ${GAIA_PROD_BUILD}/db/core
    -o ${GAIA_GENERATED_SCHEMAS}
    -n ${INCUBATOR_INSTANCE_NAME}
    -d barn_storage
    -g ${INCUBATOR_DDL}
  DEPENDS gaiac
  DEPENDS ${INCUBATOR_DDL}
)

add_custom_target(generate_incubator_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_barn_storage.h
)
