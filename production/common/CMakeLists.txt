#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(common)

set(GAIA_COMMON_INCLUDES
  "${GAIA_INC}")

set(GAIA_MULTI_PROCESS_INCLUDES
  ${GAIA_COMMON_INCLUDES}
  ${GAIA_REPO}/production/db/core/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
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
  src/random.cpp
  src/gaia_version.cpp
)

configure_gaia_target(gaia_common)
add_dependencies(gaia_common flatbuffers_dependency)
target_include_directories(gaia_common PUBLIC ${GAIA_COMMON_INCLUDES} ${FLATBUFFERS_INC})

# The following are INTERFACE libraries for including headers.
target_link_libraries(gaia_common PUBLIC cpptoml)
target_link_libraries(gaia_common PUBLIC gaia_spdlog)
target_link_libraries(gaia_common PUBLIC gaia_spdlog_setup)

# The logging unit test needs a logger configuration file.
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")

# Tests.
add_gtest(test_assert tests/test_assert.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_int_type tests/test_int_type.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_queue tests/test_queue.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_multi_process tests/test_multi_process.cpp "${GAIA_MULTI_PROCESS_INCLUDES}" "rt;gaia_direct;gaia_db_catalog_test;dac_addr_book")
add_gtest(test_logging "tests/test_logging.cpp;${GAIA_REPO}/production/common/src/debug_logger.cpp" "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_random tests/test_random.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_inline_shared_lock tests/test_inline_shared_lock.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_hash tests/test_hash.cpp "${GAIA_COMMON_INCLUDES}" "")
