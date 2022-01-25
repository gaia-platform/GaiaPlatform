#############################################
# Copyright (c) 2021 Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
#############################################

cmake_minimum_required(VERSION 3.16)

project(hospital)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${PROJECT_SOURCE_DIR}/hospital.ddl
  DATABASE_NAME hospital
)

add_executable(hospital
    hospital.cpp
)

target_add_gaia_generated_sources(hospital)
target_include_directories(hospital PRIVATE ${GAIA_INC})
target_link_libraries(hospital PRIVATE ${GAIA_LIB} Threads::Threads)
