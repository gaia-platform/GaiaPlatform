#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(catalog_schema)

string(RANDOM GAIAC_CATALOG_INSTANCE_NAME)

# We generate and copy the catalog DAC code into the gaia source tree compiling it as a starndard cpp file.
# There are 2 reasons for this:
# 1. We don't want the catalog DAC API to change and possibly break on every modification to the catalog code.
# 2. We use the catalog generated code to "observe" the changes in the DAC generation logic.

# We use a custom command to generate catalog DAC code because process_schema_internal()
# creates dependencies on the generated code which would causes this target to be automatically
# called on every change of the gaia source code (db, catalog, etc...)
add_custom_command(
    COMMENT "Generating DAC code for database catalog..."
    OUTPUT ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.cpp
    OUTPUT ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.h
    OUTPUT ${GAIA_GENERATED_CODE}/catalog/catalog_generated.h
    COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac
      -t ${GAIA_PROD_BUILD}/db/core
      -o ${GAIA_GENERATED_CODE}/catalog
      -d catalog
      -n ${GAIAC_CATALOG_INSTANCE_NAME}
      -g
)


# This is a standalone command and is not automatically run as part of the build. To run it you can use either of these
# commands:
# cmake --build build_folder --target generate_catalog_dac_code
# make generate_catalog_dac_code
add_custom_target(generate_catalog_dac_code
    COMMENT "Copying generated DAC code for catalog into Gaia source tree..."
    DEPENDS ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.cpp
    DEPENDS ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.h
    DEPENDS ${GAIA_GENERATED_CODE}/catalog/catalog_generated.h
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.cpp ${GAIA_REPO}/production/catalog/src/
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.h ${GAIA_REPO}/production/inc/gaia_internal/catalog/
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/catalog_generated.h ${GAIA_REPO}/production/inc/gaia_internal/catalog/
)

set_target_properties(generate_catalog_dac_code PROPERTIES
  EXCLUDE_FROM_ALL true)
