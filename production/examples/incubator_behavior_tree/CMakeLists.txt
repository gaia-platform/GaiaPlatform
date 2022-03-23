###################################################
# Copyright (c) Gaia Platform LLC
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

set(INCUBATOR_BEHAVIOR_TREE_DDL ${PROJECT_SOURCE_DIR}/src/incubator.ddl)
set(BEHAVIOR_TREE_DDL ${PROJECT_SOURCE_DIR}/src/behave.ddl)
set(BEHAVIOR_TREE_RULESET ${PROJECT_SOURCE_DIR}/src/behave.ruleset)
set(BEHAVIOR_TREE_INCLUDES ${PROJECT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${INCUBATOR_BEHAVIOR_TREE_DDL}
  DATABASE_NAME incubator
)

process_schema(
  DDL_FILE ${BEHAVIOR_TREE_DDL}
  DATABASE_NAME behave
)

# -- Translate ruleset into CPP --
translate_ruleset(
     RULESET_FILE ${BEHAVIOR_TREE_RULESET}
     CLANG_PARAMS -I ${BEHAVIOR_TREE_INCLUDES}})

add_executable(incubator
    src/incubator.cpp
)

target_add_gaia_generated_sources(incubator)
target_include_directories(incubator PRIVATE ${GAIA_INC} ${BEHAVIOR_TREE_INCLUDES})
target_link_directories(incubator PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(incubator PRIVATE gaia rt Threads::Threads)
