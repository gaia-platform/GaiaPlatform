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
)

target_include_directories(gaia_tools PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${GAIA_SPDLOG_INC}
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
# TODO create add_benchmark cmake function. This is mostly to show it works.

add_executable(bench_read_perf_basic
  tests/bench_read_perf_basic.cpp
  ${GAIA_REPO}/production/common/src/timer.cpp
  ${GAIA_GENERATED_CODE}/direct_access/perf_basic/gaia_perf_basic.cpp
)

target_include_directories(bench_read_perf_basic PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${GAIA_INC}
  ${GAIA_SPDLOG_INC}
  ${GAIA_GENERATED_CODE}/direct_access/perf_basic
)

target_link_libraries(bench_read_perf_basic PRIVATE
  gaia
  gtest_main
  gaia_tools
  gaia_benchmark
)

add_dependencies(bench_read_perf_basic
  dac_perf_basic
)

gtest_discover_tests(bench_read_perf_basic)
