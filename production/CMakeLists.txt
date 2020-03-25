#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

enable_testing()

# Set the project name.
project(production)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(gaia)
include(GoogleTest)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set some global variables.
get_repo_root(${PROJECT_SOURCE_DIR} GAIA_REPO)
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")
set(GAIA_INC "${PROJECT_SOURCE_DIR}/inc")
set(GOOGLE_TEST "${GAIA_REPO}/third_party/production/googletest")
set(GOOGLE_TEST_INC "${GOOGLE_TEST}/googletest/include")
set(FLATBUFFERS "${GAIA_REPO}/third_party/production/flatbuffers")
set(FLATBUFFERS_INC "${FLATBUFFERS}/include")

message(STATUS "GAIA_INC=${GAIA_INC}")

find_program(PYTHON "python")

# Common code - currently, a headers-only library.
add_library(common INTERFACE)
target_sources(common INTERFACE 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/gaia_exception.hpp 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/retail_assert.hpp
)

# Add flatbuffers
add_subdirectory("../third_party/production/flatbuffers" flatbuffers)

# Add individual component folders.
add_subdirectory(db)
add_subdirectory(rules)
add_subdirectory(tests)



# Add googletest
add_subdirectory(${GOOGLE_TEST} googletest)
