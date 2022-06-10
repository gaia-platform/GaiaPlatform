###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(benchmarks)

# gaia_tools to control the db server and gaiac.

add_library(gaia_tools
  src/gaiac_cmd.cpp
  src/db_server_cmd.cpp
  src/resource_manager.cpp
  # We can't link against gaia_common, test_perf.hpp uses timer.hpp to measure times.
  ${GAIA_REPO}/production/common/src/timer.cpp
)

target_include_directories(gaia_tools PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${GAIA_SPDLOG_INC}
  ${FLATBUFFERS_INC}
  ${GAIA_INC}
)

target_link_libraries(gaia_tools PRIVATE
  gaia
)

# Base benchmarking infra

add_library(gaia_benchmark
  src/benchmark_test_base.cpp
)

target_include_directories(gaia_benchmark PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${GAIA_SPDLOG_INC}
  ${GAIA_INC}
)

target_link_libraries(gaia_benchmark PRIVATE
  gaia
  gtest_main
  gaia_tools
)

configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")
configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")

# Copies the config files in the root cmake directory so that you can invoke the benchmarks from there.
configure_file("${GAIA_LOG_CONFIG}" "${GAIA_PROD_BUILD}/gaia_log.conf")
configure_file("${GAIA_CONFIG}" "${GAIA_PROD_BUILD}/gaia.conf")

# Benchmarks
add_benchmark(
  NAME benchmark_read_basic
  SRC_FILES tests/benchmark_read_basic.cpp
  DAC_LIB dac_perf_basic
)
