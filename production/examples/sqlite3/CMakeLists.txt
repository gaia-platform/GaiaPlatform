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

enable_testing()

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")
include(FindSQLite3)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

# -- Gaia common --
add_library(gaia_common
  src/timer.cpp
)

target_include_directories(gaia_common PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

# --- Sqlite3 benchmark ---

add_executable(sqlite3_bench
  src/sqlite3_bench.cpp
)

target_link_libraries(sqlite3_bench PRIVATE
  SQLite::SQLite3
)


add_executable(sqlite3_bench_simple
  src/sqlite3_bench_simple.cpp
)

target_include_directories(sqlite3_bench_simple PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sqlite3_bench_simple PRIVATE
  SQLite::SQLite3
  gaia_common
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(sqlite3_bench_simple)
