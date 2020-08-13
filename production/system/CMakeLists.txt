#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(system)

set(GAIA_SYSTEM_PUBLIC_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/public/db"
  "${GAIA_INC}/public/catalog"
  "${GAIA_INC}/public/direct_access"
  "${GAIA_INC}/public/system"
  "${GAIA_INC}/public/rules"
  "${GAIA_GENERATED_SCHEMAS}"
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

target_include_directories(gaia PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia PUBLIC gaia_direct gaia_rules gaia_catalog)
set_target_properties(gaia PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia PROPERTY POSITION_INDEPENDENT_CODE ON)
add_gtest(test_gaia_system "tests/test_gaia_system.cpp"
  "${GAIA_SYSTEM_PUBLIC_INCLUDES};${GAIA_SYSTEM_PRIVATE_INCLUDES}" "rt;gaia" "generate_addr_book_db_headers")
