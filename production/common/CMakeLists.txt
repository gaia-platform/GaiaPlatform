#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(common)

set(GAIA_COMMON_INCLUDES
  "${GAIA_INC}/internal/common"
  "${GAIA_INC}/public/common")

message(VERBOSE "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")

# Our main target.
add_library(gaia_common STATIC
  src/exceptions.cpp
  src/file.cpp
  src/timer.cpp
  src/logger.cpp
  src/logger_manager.cpp)
target_include_directories(gaia_common PUBLIC ${GAIA_COMMON_INCLUDES})

find_package(fmt REQUIRED)
target_link_libraries(gaia_common fmt::fmt)

find_package(spdlog REQUIRED)
target_link_libraries(gaia_common spdlog::spdlog)

target_link_libraries(gaia_common spdlog_setup)

# SYSLOG logging is temporarily disabled: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-387
# (To enable syslog logging -DSPDLOG_ENABLE_SYSLOG)
set_target_properties(gaia_common PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -DSPDLOG_FMT_EXTERNAL")

# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_common PROPERTY POSITION_INDEPENDENT_CODE ON)

# Tests.
add_gtest(test_retail_assert tests/test_retail_assert.cpp "${GAIA_COMMON_INCLUDES}" "")
add_gtest(test_queue tests/test_queue.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_logging tests/test_logging.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
