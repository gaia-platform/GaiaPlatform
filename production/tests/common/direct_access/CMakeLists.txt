#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(direct_access)

set(GAIA_TEST_COMMON_INCLUDES 
    ${GAIA_INC}/public/common
    ${GAIA_INC}/internal/common
    ${GAIA_INC}/public/rules
    ${GAIA_RULES_INC}
    ${PROJECT_SOURCE_DIR}/inc
    ${FLATBUFFERS_INC}
)
message(STATUS "GAIA_TEST_COMMON_INCLUDES=${GAIA_TEST_COMMON_INCLUDES}")

# generate the headers from the flatbuffer
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp(addr_book.fbs)
get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(test_generated_code
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

# unit tests
set (DIRECT_ACCESS_TEST_INCLUDES
    ${GAIA_TEST_COMMON_INCLUDES}
    ${GAIA_REPO}/production/db/storage_engine/mock
)
add_gtest(test_direct_access test_direct_access.cpp "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia" "test_generated_code")
