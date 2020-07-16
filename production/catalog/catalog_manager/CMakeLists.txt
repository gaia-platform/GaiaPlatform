#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

project(catalog_manager)

set(CATALOG_FBS_GENERATED_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/rules/event_manager/generated")

set(GAIA_CATALOG_LIB_INCLUDES
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_REPO}/production/db/inc/triggers
  ${GAIA_REPO}/production/rules/event_manager/inc
  ${GAIA_REPO}/production/inc/public/rules
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/internal/common
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${CATALOG_FBS_GENERATED_OUTPUTS}
  ${GENERATED_OUTPUTS}
)
message(STATUS "GAIA_CATALOG_LIB_INCLUDES=${GAIA_CATALOG_LIB_INCLUDES}")

# Generate the catalog headers from the flatbuffer schema.
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp_opt(flatbuffers/catalog.fbs
  "--no-includes;--gen-compare;--gaia-type-initial-value;6"
  "${CATALOG_FBS_GENERATED_OUTPUTS}")
get_generated_output(catalog_fbs_generated)
if(catalog_fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_catalog_edc
    DEPENDS ${catalog_fbs_generated}
    COMMENT "All generated files were updated.")
endif()

# main target
add_library(gaia_catalog STATIC
  src/catalog_manager.cpp
  src/fbs_generator.cpp
)
add_dependencies(gaia_catalog generate_catalog_edc generate_event_logs)
set_target_properties(gaia_catalog PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(gaia_catalog PRIVATE ${GAIA_CATALOG_LIB_INCLUDES})

add_gtest(test_catalog_manager "tests/test_catalog_manager.cpp" "${GAIA_CATALOG_LIB_INCLUDES}" "rt;gaia;gaia_catalog;gaia_triggers")
add_gtest(test_fbs_generation "tests/test_fbs_generation.cpp" "${GAIA_CATALOG_LIB_INCLUDES}" "rt;gaia;gaia_catalog;flatbuffers;gaia_triggers")
