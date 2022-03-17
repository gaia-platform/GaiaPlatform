###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(hospital)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(GAIA_SOURCE "/home/dax/git/GaiaPlatform/production")
set(GAIA_BUILD "${GAIA_SOURCE}/build")

set(GAIA_INC "${GAIA_SOURCE}/inc")
set(GAIA_GAIAT_CMD "${GAIA_BUILD}/tools/gaia_translate/gaiat")
set(GAIA_GAIAC_CMD "${GAIA_BUILD}/catalog/gaiac/gaiac")
set(GAIA_LIB_DIR "${GAIA_BUILD}/sdk")

# Use a custom gaia config file to suppress rule stats logging.
configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia_tutorial.conf")

include("${GAIA_SOURCE}/cmake/gaia.cmake")

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${PROJECT_SOURCE_DIR}/hospital.ddl
  DATABASE_NAME hospital
)

translate_ruleset(
  RULESET_FILE ${PROJECT_SOURCE_DIR}/hospital.ruleset
  CLANG_PARAMS -I${GAIA_BUILD}/deps/gaia_spdlog/include -I${GAIA_BUILD}/deps/flatbuffers/include
)

add_executable(hospital
  hospital.cpp
  lesson_manager.cpp
)

target_add_gaia_generated_sources(hospital)
target_include_directories(hospital PRIVATE
  ${GAIA_INC} ${GAIA_BUILD}/deps/gaia_spdlog/include
  ${GAIA_BUILD}/deps/flatbuffers/include
  ${PROJECT_SOURCE_DIR})
target_link_directories(hospital PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(hospital PRIVATE gaia Threads::Threads)
