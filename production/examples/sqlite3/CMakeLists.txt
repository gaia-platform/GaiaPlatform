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

FetchContent_Declare(
  sqlite_orm
  URL https://github.com/fnc12/sqlite_orm/archive/refs/tags/v1.7.1.zip
)
FetchContent_MakeAvailable(sqlite_orm)

include(FetchContent)
FetchContent_Declare(
  objectbox
  GIT_REPOSITORY https://github.com/objectbox/objectbox-c.git
  GIT_TAG        v0.14.0
)

FetchContent_MakeAvailable(objectbox)

# -- Gaia common --
add_library(gaia_common
  src/timer.cpp
)

target_include_directories(gaia_common PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

# --- Sqlite3 benchmark ---

add_executable(sqlite3_bench_c_api
  src/sqlite3_bench_c_api.cpp
)

target_include_directories(sqlite3_bench_c_api PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sqlite3_bench_c_api PRIVATE
  SQLite::SQLite3
#  /usr/lib/x86_64-linux-gnu/libsqlite3.a
#  dl
  gaia_common
  gtest_main
)

add_executable(sqlite3_bench_orm
  src/sqlite3_bench_orm.cpp
)

target_include_directories(sqlite3_bench_orm PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sqlite3_bench_orm PRIVATE
  SQLite::SQLite3
  gaia_common
  gtest_main
  sqlite_orm::sqlite_orm
)

add_executable(objectbox_bench
  src/objectbox_bench.cpp
  src/objectbox/simple-table.obx.cpp
)

target_include_directories(objectbox_bench PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/objectbox
)

target_link_libraries(objectbox_bench PRIVATE
  SQLite::SQLite3
  gaia_common
  gtest_main
  objectbox
)

include(GoogleTest)
gtest_discover_tests(
  sqlite3_bench_c_api
  sqlite3_bench_orm
  objectbox_bench
)
