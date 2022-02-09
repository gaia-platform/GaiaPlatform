###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(rules_vlr)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Generate Direct Access classes from the DDL schema.
process_schema(
  DDL_FILE ${PROJECT_SOURCE_DIR}/rules_vlr.ddl
  DATABASE_NAME rules_vlr
)

# Translate the ruleset into C++.
translate_ruleset(
  RULESET_FILE ${PROJECT_SOURCE_DIR}/rules_vlr.ruleset
)

add_executable(rules_vlr
  rules_vlr.cpp
)

target_add_gaia_generated_sources(rules_vlr)
target_include_directories(rules_vlr PRIVATE ${GAIA_INC})
target_link_libraries(rules_vlr PRIVATE ${GAIA_LIB} Threads::Threads)
