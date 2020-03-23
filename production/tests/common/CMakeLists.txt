#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
enable_testing()

# Set the project name.
project(direct_access)

set(GAIA_TEST_COMMON_INCLUDES 
    ${GAIA_INC}/public/common
    ${GAIA_INC}/internal/common
    ${GAIA_INC}/public/rules
    ${GAIA_RULES_INC}
    ${PROJECT_SOURCE_DIR}/inc
)

message(STATUS "GAIA_TEST_COMMON_INCLUDES=${GAIA_TEST_COMMON_INCLUDES}")

# unit tests
add_executable(test_direct_access direct_access/test_direct_access.cpp)
set_target_properties(test_direct_access PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")

target_include_directories(test_direct_access PRIVATE 
    ${GAIA_TEST_COMMON_INCLUDES}
    ${GAIA_REPO}/production/db/storage_engine/mock
    ${GOOGLE_TEST_INC}
)

target_link_libraries(test_direct_access PRIVATE 
    gtest_main
    rt
)

gtest_discover_tests(test_direct_access)
