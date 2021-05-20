#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(index_sandbox_schema)

set(INDEX_SANDBOX_DDL "${PROJECT_SOURCE_DIR}/index_sandbox.ddl")

#
# Generate stand alone schema for gaia index.
#
add_custom_command(
  COMMENT "Compiling index_sandbox.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_index_sandbox.h
  COMMAND ${GAIA_GENERATE} --destroy-db -t ${GAIA_PROD_BUILD}/db/core -o ${GAIA_GENERATED_SCHEMAS} -d index_sandbox -g ${INDEX_SANDBOX_DDL}
  DEPENDS gaiac
  DEPENDS ${INDEX_SANDBOX_DDL}
)

add_custom_target(generate_index_sandbox_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_index_sandbox.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_incubator_headers
)
