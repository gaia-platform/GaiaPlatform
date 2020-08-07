#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(catalog_schema)

add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_catalog.h
  COMMAND cmake -E make_directory "${GAIA_GENERATED_SCHEMAS}"
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GAIA_GENERATED_SCHEMAS} ${CATALOG_DDL}
  DEPENDS gaiac
  DEPENDS ${CATALOG_DDL}
  COMMENT "Compiled ${CATALOG_DDL}"
)

add_custom_target(generate_catalog_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_catalog.h
)
