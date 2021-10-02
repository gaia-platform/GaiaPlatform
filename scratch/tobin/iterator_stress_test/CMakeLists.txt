#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(iterator_stress_test)

set(CMAKE_CXX_STANDARD 17)

# Ensure debug builds have the DEBUG flag defined.
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include("/opt/gaia/cmake/gaia.cmake")

# Default compiler/linker flags.
add_compile_options(-Wall -Wextra)

# Debug build-specific options.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # We build the SDK with libc++ in debug mode.
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
  add_compile_options(-O0 -g3 -ggdb -fno-omit-frame-pointer -fno-optimize-sibling-calls -ggnu-pubnames -gsplit-dwarf)
  if(SANITIZER STREQUAL "ASAN")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
  endif()
else()
  add_compile_options(-O3)
endif()

set(ITERATOR_STRESS_TEST_DDL ${PROJECT_SOURCE_DIR}/iterator_stress_test.ddl)
# set(HELLO_RULESET ${PROJECT_SOURCE_DIR}/iterator_stress_test.ruleset)

# --- Generate Direct Access classes from DDL---
process_schema(
  DDL_FILE ${ITERATOR_STRESS_TEST_DDL}
  DATABASE_NAME iterator_stress_test
)

# -- Translate ruleset into CPP --
# translate_ruleset(RULESET_FILE ${ITERATOR_STRESS_TEST_RULESET})

add_executable(iterator_stress_test
  iterator_stress_test.cpp
)

target_add_gaia_generated_sources(iterator_stress_test)
target_include_directories(iterator_stress_test PRIVATE ${GAIA_INC})
target_link_directories(iterator_stress_test PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(iterator_stress_test PRIVATE gaia rt Threads::Threads)
