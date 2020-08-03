#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(direct_access)

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/direct_access/generated")

set(GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES
    ${GAIA_INC}/public/direct_access
    ${GAIA_INC}/public/db
    ${GAIA_INC}/public/common
)
message(STATUS "GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES=${GAIA_DIRECT_ACCESS_PUBLIC_INCLUDES}")

set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

set(GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/internal/db
  ${FLATBUFFERS_INC}
  ${PROJECT_SOURCE_DIR}
  ${GENERATED_OUTPUTS}
)
message(STATUS "GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES=${GAIA_DIRECT_ACCESS_PRIVATE_INCLUDES}")

# Our main target.
add_library(gaia_direct STATIC
  src/gaia_base.cpp
  src/auto_transaction.cpp
)

# Generate the headers from the Gaia DDL
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_addr_book.h
  COMMAND cmake -E make_directory "${GENERATED_OUTPUTS}"
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/addr_book.ddl
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/addr_book.ddl
  COMMENT "Compiling addr_book.ddl."
)

add_custom_target(addr_book_headers_generated
  DEPENDS ${GENERATED_OUTPUTS}/gaia_addr_book.h
)

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

add_gtest(test_direct_access "tests/test_direct_access.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct" "addr_book_headers_generated")
add_gtest(test_references "tests/test_references.cpp" "${DIRECT_ACCESS_TEST_INCLUDES}" "rt;gaia_direct" "addr_book_headers_generated")
