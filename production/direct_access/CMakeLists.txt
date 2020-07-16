#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(direct_access)

set(GAIA_DIRECT_ACCESS_INCLUDES 
    "${GAIA_INC}/public/direct_access"
    "${GAIA_INC}/public/common"
    "${GAIA_INC}/internal/common"
    "${FLATBUFFERS_INC}"
    "${GAIA_REPO}/production/db/storage_engine/mock"
    .
)
message(STATUS "GAIA_DIRECT_ACCESS_INCLUDES=${GAIA_DIRECT_ACCESS_INCLUDES}")

# Our main target.
add_library(gaia_direct STATIC 
  src/gaia_object_defs.cpp
  src/gaia_base.cpp
)

# Generate the headers from the flatbuffer
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp(tests/addr_book.fbs)
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(test_generated_code
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()


target_include_directories(gaia_direct PRIVATE ${GAIA_DIRECT_ACCESS_INCLUDES})

set_target_properties(gaia_direct PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_property (TARGET gaia_direct PROPERTY POSITION_INDEPENDENT_CODE ON)



# Start unit tests.
set (DIRECT_ACCESS_TEST_INCLUDES
    ${GAIA_DIRECT_ACCESS_INCLUDES}
    .
)
add_gtest(test_direct_access "tests/test_direct_access.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia" "test_generated_code")
add_gtest(test_references "tests/test_references.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia" "test_generated_code")

