#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

project(catalog_manager)

set(GENERATED_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(GAIA_CATALOG_LIB_INCLUDES
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${GENERATED_OUTPUTS}
)
message(STATUS "GAIA_CATALOG_LIB_INCLUDES=${GAIA_CATALOG_LIB_INCLUDES}")

# Generate the event log headers from the flatbuffer schema.
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp_opt(flatbuffers/catalog.fbs
  "--no-includes;--gen-compare;--gaia-type-initial-value;6"
  "${GENERATED_OUTPUTS}")
get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_catalog
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

# main target
add_library(gaia_catalog STATIC
  src/catalog_manager.cpp
)
add_dependencies(gaia_catalog generate_catalog)
set_target_properties(gaia_catalog PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(gaia_catalog PRIVATE ${GAIA_CATALOG_LIB_INCLUDES})
