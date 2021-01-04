#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(common)

set(GAIA_COMMON_INCLUDES
  "${GAIA_INTERNAL_INC}/common"
  "${GAIA_INC}")

set(GAIA_MULTI_PROCESS_INCLUDES
  ${GAIA_COMMON_INCLUDES}
  ${GAIA_INTERNAL_INC}/db
  ${GAIA_REPO}/production/db/storage_engine/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${FLATBUFFERS_INC}
  ${GAIA_GENERATED_SCHEMAS}
)

message(VERBOSE "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")
message(VERBOSE "GAIA_MULTI_PROCESS_INCLUDES=${GAIA_MULTI_PROCESS_INCLUDES}")

# Our main target.
add_library(gaia_common STATIC
  src/exceptions.cpp
  src/file.cpp
  src/timer.cpp
  src/logger.cpp
  src/logger_manager.cpp)
target_include_directories(gaia_common PUBLIC ${GAIA_COMMON_INCLUDES})

# The following are INTERFACE libraries for including headers.
target_link_libraries(gaia_common cpptoml)
target_link_libraries(gaia_common fmt)
target_link_libraries(gaia_common spdlog)
target_link_libraries(gaia_common spdlog_setup)
target_link_libraries(gaia_common stdc++fs)

# SYSLOG logging is temporarily disabled: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-387
# (To enable syslog logging -DSPDLOG_ENABLE_SYSLOG)
set_target_properties(gaia_common PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_common PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_common PROPERTY POSITION_INDEPENDENT_CODE ON)

# The logging unit test needs a logger configuration file.
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

# Tests.
add_gtest(test_retail_assert tests/test_retail_assert.cpp "${GAIA_COMMON_INCLUDES}" "")
add_gtest(test_queue tests/test_queue.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_multi_process tests/test_multi_process.cpp "${GAIA_MULTI_PROCESS_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test" "generate_addr_book_headers")
add_gtest(test_logging tests/test_logging.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
