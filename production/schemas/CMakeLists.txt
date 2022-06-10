###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(schemas)

set(GAIA_SCHEMAS_INC "${PROJECT_SOURCE_DIR}")

#
# Catalog tool we use to generate headers.
#
set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

file(MAKE_DIRECTORY ${GAIA_GENERATED_CODE})

file(GLOB SCHEMA_FILES
  "${GAIA_REPO}/production/schemas/test/**/*.ddl"
)

# Copies the .dll files into a known location in the Cmake build folder
# to allow schema_loader to find them.
foreach(SCHEMA_FILE ${SCHEMA_FILES})
  configure_file(${SCHEMA_FILE} ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

add_subdirectory(loader)
add_subdirectory(system)
add_subdirectory(test)
