#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(hello)

set(CMAKE_CXX_STANDARD 17)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Default compiler/linker flags.
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -O3")
set(GAIA_LINK_FLAGS " ")

# Debug build-specific options.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -O0 -g -fno-limit-debug-info")
  if(CACHE{SANITIZER} STREQUAL "ASAN")
    set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer")
    set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all")
  endif()
endif()

include(/opt/gaia/cmake/gaia.cmake)

set(HELLO_DDL ${PROJECT_SOURCE_DIR}/hello.ddl)
set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/hello.ruleset)

# --- Generate EDC from DDL---
process_schema(DDL_FILE ${HELLO_DDL})

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${HELLO_RULESET})

add_executable(hello
  hello.cpp
)

target_add_gaia_generated_sources(hello)
set_target_properties(hello PROPERTIES COMPILE_FLAGS ${GAIA_COMPILE_FLAGS})
set_target_properties(hello PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
target_include_directories(hello PRIVATE ${GAIA_INC})
target_link_libraries(hello PRIVATE gaia rt Threads::Threads)
