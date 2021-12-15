#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

project(counter)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${PROJECT_SOURCE_DIR}/counter.ddl
  DATABASE_NAME counter
)

add_executable(counter
  counter.cpp
)

target_add_gaia_generated_sources(counter)
target_include_directories(counter PRIVATE ${GAIA_INC})
target_link_libraries(counter PRIVATE ${GAIA_LIB} Threads::Threads)
