###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(hello)

set(CMAKE_CXX_STANDARD 17)

# Ensure debug builds have the DEBUG flag defined.
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

# Debug build-specific options.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # We build the SDK with libc++ in debug mode.
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
  add_compile_options(-O0 -g3 -ggdb -fno-omit-frame-pointer -fno-optimize-sibling-calls -ggnu-pubnames -gsplit-dwarf)
  if("$CACHE{SANITIZER}" STREQUAL "ASAN")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
  endif()
else()
  add_compile_options(-O3)
endif()

if(NOT DEFINED GAIA_ROOT)
  set(GAIA_ROOT "/opt/gaia")
endif()
include("${GAIA_ROOT}/cmake/gaia.cmake")

set(HELLO_DDL ${PROJECT_SOURCE_DIR}/hello.ddl)
set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/hello.ruleset)

# --- Generate Direct Access classes from DDL---
process_schema(DDL_FILE ${HELLO_DDL})

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${HELLO_RULESET})

add_executable(hello
  hello.cpp
)

target_add_gaia_generated_sources(hello)
target_include_directories(hello PRIVATE ${GAIA_INC})
target_link_directories(hello PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(hello PRIVATE gaia rt Threads::Threads)
