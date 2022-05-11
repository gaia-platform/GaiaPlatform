#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

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
  gaia_db_client
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
  NAME bench_read_perf_basic
  SRC_FILES tests/bench_read_perf_basic.cpp
  DAC_LIB dac_perf_basic
)
