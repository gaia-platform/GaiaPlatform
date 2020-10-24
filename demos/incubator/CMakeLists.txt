#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(incubator_demo)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/incubator/generated")

set(GAIA_INCUBATOR_DEMO_INCLUDES
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/system
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/internal/rules
  ${GAIA_INC}/public/db
  ${GAIA_INC}/internal/db
  ${FLATBUFFERS_INC}
  ${GENERATED_OUTPUTS}
)

add_custom_target(make_generated_incubator_dir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUTPUTS}
)

set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp_opt(barn_storage.fbs "--no-includes;--gen-compare;--gaia-type-initial-value;1000" "${GENERATED_OUTPUTS}")
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_barn_storage
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

add_library(gaia_semock SHARED IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

add_executable(incubator 
    main.cpp
    rules.cpp
    "${GENERATED_OUTPUTS}/rules_init.cpp"
)

add_dependencies(incubator generate_barn_storage)
set_target_properties(incubator PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(incubator PRIVATE ${GAIA_INCUBATOR_DEMO_INCLUDES})
target_link_libraries(incubator PRIVATE rt pthread gaia gaia_semock gaia_rules gaia_direct)
