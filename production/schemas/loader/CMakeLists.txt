###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(schema_loader)

set(GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES
  ${GAIA_INC}
  ${PROJECT_SOURCE_DIR}/inc
)

set(GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PARSER_GENERATED}
)

message(VERBOSE "GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES=${GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES}")
message(VERBOSE "GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES=${GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES}")

add_library(gaia_schema_loader STATIC src/schema_loader.cpp)

configure_gaia_target(gaia_schema_loader)

target_include_directories(gaia_schema_loader PUBLIC ${GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES})
target_include_directories(gaia_schema_loader PRIVATE ${GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES})
target_link_libraries(gaia_schema_loader PUBLIC gaia_catalog gaia_parser gaia_common)
