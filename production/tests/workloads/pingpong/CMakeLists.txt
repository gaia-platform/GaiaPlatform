###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(pingpong)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

set(MINK_DDL ${PROJECT_SOURCE_DIR}/pingpong.ddl)
set(MINK_RULESET ${PROJECT_SOURCE_DIR}/pingpong.ruleset)

# --- Generate DAC from DDL---
process_schema(
  DDL_FILE ${MINK_DDL}
  DATABASE_NAME pingpong
)

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${MINK_RULESET})

add_executable(pingpong
    pingpong.cpp
    simulation.cpp
)

target_add_gaia_generated_sources(pingpong)
target_include_directories(
  pingpong
  PUBLIC
    ${PROJECT_SOURCE_DIR}
  PRIVATE
    ${GAIA_INC})
target_link_libraries(pingpong ${GAIA_LIB})

#if(ENABLE_STACKTRACE)
#    target_link_libraries(pingpong PRIVATE gaia_stack_trace)
#endif()
