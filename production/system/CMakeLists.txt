#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(system)

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/system/generated")

set(GAIA_SYSTEM_PUBLIC_INCLUDES
    "${GAIA_INC}/public/common"
    "${GAIA_INC}/public/db"
    "${GAIA_INC}/public/catalog"
    "${GAIA_INC}/public/direct_access"
    "${GAIA_INC}/public/system"
    "${GAIA_INC}/public/rules"
    "${GENERATED_OUTPUTS}"
)
message(STATUS "GAIA_SYSTEM_PUBLIC_INCLUDES=${GAIA_SYSTEM_PUBLIC_INCLUDES}")

set(GAIA_SYSTEM_PRIVATE_INCLUDES
    "${GAIA_INC}/internal/common"
    "${GAIA_INC}/internal/db"
    "${FLATBUFFERS_INC}"
    "${GAIA_REPO}/production/rules/event_manager/inc"
)
message(STATUS "GAIA_SYSTEM_PRIVATE_INCLUDES=${GAIA_SYSTEM_PRIVATE_INCLUDES}")

# Our main target.
add_library(gaia STATIC src/gaia_system.cpp)

gaia_make_db_ddl(${CMAKE_CURRENT_SOURCE_DIR}/tests/addr_book.ddl ${GENERATED_OUTPUTS}/system_db.ddl)

set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

# Generate the headers for the integration test
add_custom_command(
  OUTPUT "gaia_system_db.h"
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS}
    ${GENERATED_OUTPUTS}/system_db.ddl
  DEPENDS ${GAIA_GENERATE}
  COMMENT "Compiling system_db.ddl."
)

add_custom_target(generate_system_db
  DEPENDS "gaia_system_db.h"
)

target_include_directories(gaia PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia PUBLIC gaia_direct gaia_rules gaia_catalog)
set_target_properties(gaia PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia PROPERTY POSITION_INDEPENDENT_CODE ON)
add_gtest(test_gaia_system "tests/test_gaia_system.cpp" "${GAIA_SYSTEM_PUBLIC_INCLUDES};${GAIA_SYSTEM_PRIVATE_INCLUDES}" "rt;gaia" "generate_system_db")
