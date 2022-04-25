###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(rules)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Use a custom gaia config file to suppress rule stats logging.
configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia_tutorial.conf")

include("/opt/gaia/cmake/gaia.cmake")

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${PROJECT_SOURCE_DIR}/clinic.ddl
  DATABASE_NAME clinic
)

translate_ruleset(
  RULESET_FILE ${PROJECT_SOURCE_DIR}/clinic.ruleset
)

add_executable(rules
  clinic.cpp
  lesson_manager.cpp
)

target_add_gaia_generated_sources(rules)
target_include_directories(rules PRIVATE ${GAIA_INC} ${PROJECT_SOURCE_DIR})
target_link_libraries(rules PRIVATE ${GAIA_LIB} Threads::Threads)
