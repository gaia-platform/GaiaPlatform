#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(catalog_schema)

string(RANDOM GAIAC_CATALOG_INSTANCE_NAME)

# We use a custom command to generate catalog EDC code because process_schema_internal()
# creates dependencies on the generated code which would causes this target to be automatically
# called.
add_custom_command(
    COMMENT "Generating EDC code for database catalog..."
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

# We copy the catalog EDC code into the gaia source tree and compile it as a normal cpp file.
# There are 2 reasons for this:
# 1. We don't want the catalog EDC API to change and possibly break on every modification to the catalog code.
# 2. We use the catalog generated code to "observe" the changes in the EDC generation logic.
#
# This is a standalone command and is not automatically run as part of the build. To run it you can use either of these
# commands:
# cmake --build build_folder --target generate_catalog_edc_code
# make generate_catalog_edc_code
add_custom_target(generate_catalog_edc_code
    COMMENT "Copying generated EDC code for catalog into Gaia source tree..."
    DEPENDS ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.cpp
    DEPENDS ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.h
    DEPENDS ${GAIA_GENERATED_CODE}/catalog/catalog_generated.h
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.cpp ${GAIA_REPO}/production/catalog/src/
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/gaia_catalog.h ${GAIA_REPO}/production/inc/gaia_internal/catalog/
    COMMAND cp ${GAIA_GENERATED_CODE}/catalog/catalog_generated.h ${GAIA_REPO}/production/inc/gaia_internal/catalog/
    # Removes the generated files to force the next invocation of this command to regenerate the files.
    COMMAND rm -r ${GAIA_GENERATED_CODE}/catalog
)

set_target_properties(generate_catalog_edc_code PROPERTIES
  EXCLUDE_FROM_ALL true)
