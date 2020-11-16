#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(schema_repository)

set(GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/catalog
  ${PROJECT_SOURCE_DIR}/inc
)

set(GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PARSER_GENERATED}
)

message(VERBOSE "GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES=${GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES}")
message(VERBOSE "GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES=${GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES}")

add_library(gaia_schema_loader STATIC src/schema_loader.cpp)

set_target_properties(gaia_schema_loader PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_schema_loader PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

target_include_directories(gaia_schema_loader PUBLIC ${GAIA_SCHEMA_LOADER_PUBLIC_INCLUDES})
target_include_directories(gaia_schema_loader PRIVATE ${GAIA_SCHEMA_LOADER_PRIVATE_INCLUDES})
target_link_libraries(gaia_schema_loader PUBLIC gaia_catalog gaia_parser gaia_common)
