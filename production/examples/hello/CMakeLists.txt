#############################################
# Copyright (c) 2021 Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
#############################################

cmake_minimum_required(VERSION 3.16)

project(hello)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

set(HELLO_DDL ${PROJECT_SOURCE_DIR}/hello.ddl)
set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/hello.ruleset)

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${HELLO_DDL}
  DATABASE_NAME hello
)

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${HELLO_RULESET})

add_executable(hello
  hello.cpp
)

target_add_gaia_generated_sources(hello)
target_include_directories(hello PRIVATE ${GAIA_INC})
target_link_directories(hello PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(hello PRIVATE gaia rt Threads::Threads)
