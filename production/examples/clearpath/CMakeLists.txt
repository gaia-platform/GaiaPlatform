###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(clearpath)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")
include(FindSQLite3)

# --- Generate Direct Access classes from DDL ---
process_schema(
  DDL_FILE ${PROJECT_SOURCE_DIR}/gaia/clearpath.ddl
  DATABASE_NAME clearpath
)

# --- Clearpath ---

add_executable(clearpath
  src/main.cpp
  src/graph.cpp
)

target_add_gaia_generated_sources(clearpath)

target_include_directories(clearpath PRIVATE
  ${GAIA_INC}
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(clearpath PRIVATE
  ${GAIA_LIB}
  Threads::Threads
)

# --- Gaia Benchmark ---

add_executable(gaia_bench
  src/gaia_bench.cpp
)

target_include_directories(gaia_bench PRIVATE
  ${GAIA_INC}
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(gaia_bench PRIVATE
  ${GAIA_LIB}
  Threads::Threads
)

target_add_gaia_generated_sources(gaia_bench)

# --- Sqlite3 benchmark ---

add_executable(sqlite3_bench
  src/sqlite3_bench.cpp
)

target_link_libraries(sqlite3_bench PRIVATE
  SQLite::SQLite3
)
