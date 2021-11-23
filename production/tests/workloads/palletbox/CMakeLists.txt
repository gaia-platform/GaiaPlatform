#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(palletbox)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

set(PALLETBOX_DDL ${PROJECT_SOURCE_DIR}/palletbox.ddl)
set(PALLETBOX_RULESET ${PROJECT_SOURCE_DIR}/palletbox.ruleset)

# --- Generate DAC from DDL---
process_schema(
  DDL_FILE ${PALLETBOX_DDL}
  DATABASE_NAME palletbox
)

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${PALLETBOX_RULESET})

add_executable(palletbox
    palletbox.cpp
    simulation.cpp
)

target_add_gaia_generated_sources(palletbox)
target_include_directories(
  palletbox
  PUBLIC
    ${PROJECT_SOURCE_DIR}
  PRIVATE
    ${GAIA_INC})
target_link_libraries(palletbox ${GAIA_LIB})

#if(ENABLE_STACKTRACE)
#    target_link_libraries(palletbox PRIVATE gaia_stack_trace)
#endif()
