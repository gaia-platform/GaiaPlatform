#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(incubator_demo)

set(GAIA_INCUBATOR_DEMO_INCLUDES
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/rules
  ${GAIA_RULES_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${FLATBUFFERS_INC}
)
message(STATUS "GAIA_INCUBATOR_DEMO_INCLUDES=${GAIA_INCUBATOR_DEMO_INCLUDES}")

# Generate the data class headers from the flatbuffer schema.
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp(barn_storage.fbs)
get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_incubator_schema
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

add_executable(demo_inc main.cpp)
add_dependencies(demo_inc generate_incubator_schema)
set_target_properties(demo_inc PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(demo_inc PRIVATE ${GAIA_INCUBATOR_DEMO_INCLUDES})
target_link_libraries(demo_inc PRIVATE rt gaia gaia_semock gaia_rules gaia_direct)
