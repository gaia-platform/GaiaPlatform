#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(common)

find_package(Threads REQUIRED)

# Add spdlog
#ExternalProject_Add(spdlog
#        PREFIX spdlog
#        SOURCE_DIR ${SPDLOG}
#        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
#        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
#        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
#        -DCMAKE_INSTALL_PREFIX=${GAIA_PROD_BUILD}/spdlog
#        -DSPDLOG_BUILD_SHARED=ON
#)

#set(spdlog_DIR ${SPDLOG}/_builds)
find_package(spdlog PATHS ${SPDLOG}/lib/lib/cmake/spdlog/ NO_DEFAULT_PATH REQUIRED)

# Add spdlog_setup
set(spdlog_setup_DIR ${SPDLOG_SETUP}/build-release/install/lib/cmake/spdlog_setup/)
find_package(spdlog_setup REQUIRED)


set(GAIA_COMMON_INCLUDES
  "${GAIA_INC}/internal/common"
  "${GAIA_INC}/public/common")

message(STATUS "GAIA_COMMON_INCLUDES=${GAIA_COMMON_INCLUDES}")

# Our main target.
add_library(gaia_common STATIC
    src/exceptions.cpp
    src/file.cpp
    src/timer.cpp
    src/gaia_logging.cpp)

add_executable(logging
        src/logging.cpp)

set_target_properties(logging PROPERTIES COMPILE_FLAGS -DSPDLOG_ENABLE_SYSLOG
                                         COMPILE_FLAGS ${GAIA_COMPILE_FLAGS}
                                         RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

target_link_libraries(logging PRIVATE Threads::Threads spdlog::spdlog spdlog_setup::spdlog_setup)

target_include_directories(gaia_common PUBLIC ${GAIA_COMMON_INCLUDES})
target_link_libraries(gaia_common PUBLIC spdlog::spdlog spdlog_setup::spdlog_setup)

set_target_properties(gaia_common PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}"
                                             COMPILE_FLAGS -DSPDLOG_ENABLE_SYSLOG)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_common PROPERTY POSITION_INDEPENDENT_CODE ON)

# Tests.
add_gtest(test_retail_assert tests/test_retail_assert.cpp "${GAIA_COMMON_INCLUDES}" "")
add_gtest(test_queue tests/test_queue.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
add_gtest(test_logging tests/test_logging.cpp "${GAIA_COMMON_INCLUDES}" "gaia_common")
