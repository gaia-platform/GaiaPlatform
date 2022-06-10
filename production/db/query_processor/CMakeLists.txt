###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(query_processor)

# Build sources.

add_library(gaia_qp STATIC
  src/qp_operator.cpp
  src/index_scan.cpp
  src/predicate.cpp
  src/scan_generators.cpp)

configure_gaia_target(gaia_qp)

set(GAIA_QP_PRIVATE_INCLUDES
  "${GAIA_INC}"
  "${GAIA_INTERNAL_INC}/index"
  "${GAIA_REPO}/production/db/inc/core"
  "${GAIA_REPO}/production/db/inc/payload_types"
  "${GAIA_REPO}/production/db/inc/index"
  "${GAIA_REPO}/production/db/inc/query_processor"
  "${GAIA_PROD_BUILD}/db/core/generated")

target_include_directories(gaia_qp PRIVATE ${GAIA_QP_PRIVATE_INCLUDES})

target_link_libraries(gaia_qp PRIVATE gaia_db_client gaia_catalog gaia_index)

set(QP_TEST_INCLUDES
  ${GAIA_QP_PRIVATE_INCLUDES}
  ${GAIA_GENERATED_SCHEMAS}
)

add_gtest(test_qp tests/test_index_scan.cpp
      "${QP_TEST_INCLUDES}" "gaia_common;gaia_direct;gaia_db_catalog_test;gaia_payload_types;gaia_index;gaia_qp;dac_index_sandbox;flatbuffers")
