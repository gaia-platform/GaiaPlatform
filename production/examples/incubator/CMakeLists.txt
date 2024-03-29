###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(incubator)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/incubator.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/incubator.ruleset)

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${INCUBATOR_DDL}
  DATABASE_NAME incubator
)

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${INCUBATOR_RULESET})

add_executable(incubator
    incubator.cpp
)

target_add_gaia_generated_sources(incubator)
target_include_directories(incubator PRIVATE ${GAIA_INC})
target_link_directories(incubator PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(incubator PRIVATE ${GAIA_LIB} rt Threads::Threads)
