#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(pingpong)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Default compiler/linker flags.
add_compile_options(-c -Wall -Wextra -stdlib=libc++)
add_link_options(-stdlib=libc++)

# Debug build-specific options.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-O0 -g3 -ggdb -fno-limit-debug-info -fno-omit-frame-pointer -fno-optimize-sibling-calls -ggnu-pubnames -gsplit-dwarf)
  if("$CACHE{SANITIZER}" STREQUAL "ASAN")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
  endif()
else()
  add_compile_options(-O3)
endif()

include("/opt/gaia/cmake/gaia.cmake")
set(MINK_DDL ${PROJECT_SOURCE_DIR}/pingpong.ddl)
set(MINK_RULESET ${PROJECT_SOURCE_DIR}/pingpong.ruleset)

# --- Generate EDC from DDL---
process_schema(DDL_FILE ${MINK_DDL})

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${MINK_RULESET})

add_executable(pingpong
    pingpong.cpp
)

target_add_gaia_generated_sources(pingpong)
add_dependencies(pingpong translate_pingpong_ruleset)
target_include_directories(pingpong PRIVATE ${GAIA_INC})
target_link_directories(pingpong PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(pingpong PRIVATE gaia rt Threads::Threads)
