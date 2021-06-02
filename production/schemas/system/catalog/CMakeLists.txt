#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(catalog_schema)

string(RANDOM CATALOG_INSTANCE_NAME)

add_custom_command(
  COMMENT "Generating catalog direct access APIs..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_catalog.h
  COMMAND ${GAIA_GENERATE}
    -t ${GAIA_PROD_BUILD}/db/core
    -o ${GAIA_GENERATED_SCHEMAS}
    -g
    -n ${CATALOG_INSTANCE_NAME}
    -d catalog
  DEPENDS gaiac
)

add_custom_target(generate_catalog_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_catalog.h
)
