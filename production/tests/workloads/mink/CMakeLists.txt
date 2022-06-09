###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(mink)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

set(MINK_DDL ${PROJECT_SOURCE_DIR}/mink.ddl)
set(MINK_RULESET ${PROJECT_SOURCE_DIR}/mink.ruleset)

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${MINK_DDL}
  DATABASE_NAME mink
)

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${MINK_RULESET})

add_executable(mink
    mink.cpp
)

target_add_gaia_generated_sources(mink)
target_include_directories(mink PRIVATE ${GAIA_INC})
target_link_directories(mink PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(mink PRIVATE gaia rt Threads::Threads)
