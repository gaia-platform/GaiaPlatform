#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(schema_repository)

set(GAIA_SCHEMA_REPOSITORY_PUBLIC_INCLUDES
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/catalog
  ${PROJECT_SOURCE_DIR}/inc
)

set(GAIA_SCHEMA_REPOSITORY_PRIVATE_INCLUDES
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_PARSER_GENERATED}
)

message(VERBOSE "GAIA_SCHEMA_REPOSITORY_PUBLIC_INCLUDES=${GAIA_SCHEMA_REPOSITORY_PUBLIC_INCLUDES}")
message(VERBOSE "GAIA_SCHEMA_REPOSITORY_PRIVATE_INCLUDES=${GAIA_SCHEMA_REPOSITORY_PRIVATE_INCLUDES}")

add_library(schema_repository STATIC src/schema_repository.cpp)

set_target_properties(schema_repository PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(schema_repository PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

target_include_directories(schema_repository PUBLIC ${GAIA_SCHEMA_REPOSITORY_PUBLIC_INCLUDES})
target_include_directories(schema_repository PRIVATE ${GAIA_SCHEMA_REPOSITORY_PRIVATE_INCLUDES})
target_link_libraries(schema_repository PUBLIC gaia_catalog gaia_parser gaia_common)
