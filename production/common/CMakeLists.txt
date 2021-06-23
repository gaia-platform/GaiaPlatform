#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(common)

set(GAIA_COMMON_INCLUDES
  "${GAIA_INC}")

set(GAIA_MULTI_PROCESS_INCLUDES
  ${GAIA_COMMON_INCLUDES}
  ${GAIA_REPO}/production/db/core/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${FLATBUFFERS_INC}
)

message(VERBOSE "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")
message(VERBOSE "GAIA_MULTI_PROCESS_INCLUDES=${GAIA_MULTI_PROCESS_INCLUDES}")

# Our main target.
add_library(gaia_common STATIC
  src/config.cpp
  src/exceptions.cpp
  src/file.cpp
  src/timer.cpp
  src/logger.cpp
  src/logger_manager.cpp
  src/random.cpp)
target_include_directories(gaia_common PUBLIC ${GAIA_COMMON_INCLUDES})

# The following are INTERFACE libraries for including headers.
target_link_libraries(gaia_common cpptoml)
target_link_libraries(gaia_common gaia_spdlog)
target_link_libraries(gaia_common gaia_spdlog_setup)

set_target_properties(gaia_common PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_common PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

# The logging unit test needs a logger configuration file.
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

set(LOG_LEVEL_CONFIGS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/tests/logging_configs")
set(LOG_LEVEL_CONFIGS_BIN_DIR "${PROJECT_BINARY_DIR}/logging_configs")
file(MAKE_DIRECTORY "${LOG_LEVEL_CONFIGS_BIN_DIR}")

# The logging_configs directory is for setting one log level per configuration file.
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_trace.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_debug.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_info.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_warn.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_error.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_critical.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")
configure_file("${LOG_LEVEL_CONFIGS_SRC_DIR}/log_off.conf" "${LOG_LEVEL_CONFIGS_BIN_DIR}")

# Populate the gaia_version file
configure_file(${GAIA_INC}/gaia_internal/common/gaia_version.hpp.in ${GAIA_INC}/gaia_internal/common/gaia_version.hpp)

# Tests.
add_gtest(test_retail_assert tests/test_retail_assert.cpp "${GAIA_COMMON_INCLUDES}" "")
add_gtest(test_queue tests/test_queue.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_multi_process tests/test_multi_process.cpp "${GAIA_MULTI_PROCESS_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;edc_addr_book")
add_gtest(test_logging tests/test_logging.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_random tests/test_random.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
