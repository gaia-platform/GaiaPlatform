#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(catalog_schema)

process_schema_internal(DATABASE_NAME catalog
    LIB_NAME edc_catalog_discard
)

# Instead of using edc_catalog as a static library to link against the project targets, we copy the source code into
# the gaia source tree and compile it as normal cpp file. There are 2 reasons for this:
# 1. We don't want the catalog EDC API to change and possibly break on every modification to the catalog code.
# 2. We use the catalog generated code to "observe" the changes in the EDC generation logic.
#
# This is a standalone command and is not automatically run as part of the build. To run it you can:
# cmake --build build_folder --target generate_catalog_edc_code
add_custom_command(
    COMMENT "Copying generated EDC code for catalog into Gaia source tree..."
    OUTPUT ${GAIA_REPO}/catalog/src/gaia_catalog.cpp
    OUTPUT ${GAIA_REPO}/inc/gaia_internal/catalog/gaia_catalog.h
    OUTPUT ${GAIA_REPO}/inc/gaia_internal/catalog/catalog_generated.h
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.cpp ${GAIA_REPO}/production/catalog/src/
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.h ${GAIA_REPO}/production/inc/gaia_internal/catalog/
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/catalog_generated.h ${GAIA_REPO}/production/inc/gaia_internal/catalog/
    DEPENDS edc_catalog_discard
)

add_custom_target(generate_catalog_edc_code ALL
    DEPENDS ${GAIA_REPO}/catalog/src/gaia_catalog.cpp
    DEPENDS ${GAIA_REPO}/inc/gaia_internal/catalog/gaia_catalog.h
    DEPENDS ${GAIA_REPO}/inc/gaia_internal/catalog/catalog_generated.h
)
