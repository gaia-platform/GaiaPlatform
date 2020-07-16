#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(direct_access)

set(GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES
    "${GAIA_INC}/public/direct_access"
    "${GAIA_INC}/public/db"
    "${GAIA_INC}/public/common"
)
message(STATUS "GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES=${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}")

set(GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES
    "${GAIA_INC}/internal/common"
    "${GAIA_INC}/internal/db"
    "${FLATBUFFERS_INC}"
    "${PROJECT_SOURCE_DIR}"
)
message(STATUS "GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES=${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}")

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

target_include_directories(gaia_direct PUBLIC ${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES})
target_include_directories(gaia_direct PRIVATE ${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES})
target_link_libraries(gaia_direct PUBLIC gaia_semock)
set_target_properties(gaia_direct PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_direct PROPERTY POSITION_INDEPENDENT_CODE ON)

# Start unit tests.
set (DIRECT_ACCESS_TEST_INCLUDES
  ${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}
  ${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}
)
add_gtest(test_direct_access "tests/test_direct_access.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia" "test_generated_code")
add_gtest(test_references "tests/test_references.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia" "test_generated_code")
