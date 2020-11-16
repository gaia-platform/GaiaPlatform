#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(schemas)

set(GAIA_SCHEMAS_INC "${PROJECT_SOURCE_DIR}")

#
# Catalog tool we use to generate headers.
#
set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

file(MAKE_DIRECTORY ${GAIA_GENERATED_SCHEMAS})

file(GLOB SCHEMA_FILES
  "${GAIA_REPO}/production/schemas/test/**/*.ddl"
)

# Copies the .dll files into a known location in the Cmake build folder
# to allow schema_repository to find them.
file(
  COPY ${SCHEMA_FILES}
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/"
)

add_subdirectory(loader)
add_subdirectory(system)
add_subdirectory(test)
