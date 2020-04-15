#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(incubator_demo)

get_filename_component(GAIA_REPO ../../ ABSOLUTE)
set(GAIA_PROD "${GAIA_REPO}/production")
set(GAIA_INC "${GAIA_PROD}/inc")

set(FLATBUFFERS "${GAIA_REPO}/third_party/production/flatbuffers")
set(FLATBUFFERS_INC "${FLATBUFFERS}/include")

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")

set(GAIA_INCUBATOR_DEMO_INCLUDES
  ${GAIA_PROD}/db/storage_engine/mock
  ${GAIA_INC}/public/common
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/rules
  ${FLATBUFFERS_INC}
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/barn_storage_generated.h"
    COMMAND "${GAIA_PROD_BUILD}/flatbuffers/flatc"
            --cpp --gen-mutable --gen-object-api --reflect-names
            --cpp-ptr-type flatbuffers::unique_ptr
            --cpp-str-type gaia::common::nullable_string_t
            --cpp-str-flex-ctor
            --gaiacpp --gen-setters
            --no-includes --gen-compare
            --gen-table-events --gen-transaction-events --gen-col-events
            --gaia_type_initial_value 1000
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            -o ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/barn_storage.fbs
    DEPENDS ${GAIA_PROD_BUILD}/flatbuffers/flatc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/barn_storage.fbs
    COMMENT "Run generation: ${OUTPUT}"
)

add_custom_target(generate_barn_storage
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/barn_storage_generated.h"
  COMMENT "All generated files were updated.")

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

add_library(gaia_semock SHARED IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

add_executable(incubator main.cpp)
add_dependencies(incubator generate_barn_storage)
set_target_properties(incubator PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(incubator PRIVATE ${GAIA_INCUBATOR_DEMO_INCLUDES})
target_link_libraries(incubator PRIVATE rt gaia gaia_semock gaia_rules gaia_direct)

add_executable(rules rules.cpp)
add_dependencies(rules generate_barn_storage)
set_target_properties(rules PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(rules PRIVATE ${GAIA_INCUBATOR_DEMO_INCLUDES})
target_link_libraries(rules PRIVATE rt gaia gaia_semock gaia_rules gaia_direct)
