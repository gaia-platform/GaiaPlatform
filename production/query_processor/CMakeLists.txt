# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

# Set the project name.
project(query_processor)

# Build sources.

add_library(gaia_qp SHARED
  src/qp_operator.cpp
  src/index_scan.cpp)


set(GAIA_QP_PRIVATE_INCLUDES
  "${PROJECT_SOURCE_DIR}/inc"
  "${GAIA_REPO}/production/db/core/inc"
  "${GAIA_INC}"
  "${GAIA_INTERNAL_INC}/index"
  "${GAIA_REPO}/production/db/inc/memory_manager"
  "${GAIA_REPO}/production/db/inc/payload_types"
  "${GAIA_REPO}/production/db/inc/index"
  "${FLATBUFFERS_INC}")

target_include_directories(gaia_qp PRIVATE ${GAIA_QP_PRIVATE_INCLUDES})

target_link_libraries(gaia_qp PRIVATE gaia_db_client)
target_link_libraries(gaia_qp PRIVATE gaia_catalog)

# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_qp PROPERTY POSITION_INDEPENDENT_CODE ON)

set (QP_TEST_INCLUDES
  ${GAIA_QP_PRIVATE_INCLUDES}
  ${GAIA_GENERATED_SCHEMAS}
)

add_gtest(test_qp tests/test_index_scan.cpp
          "${INDEX_TEST_INCLUDES}" "gaia_common;gaia_direct;gaia_db_catalog_test;gaia_payload_types;gaia_index;flatbuffers"
          "generate_index_sandbox_headers")

add_dependencies(test_qp generate_index_sandbox_headers)
