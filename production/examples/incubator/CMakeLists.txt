#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# {SDK-SETUP:CMAKE} - Do not remove this line

# Set the project name.
project(incubator)

# {SDK-SETUP:CXX_STANDARD} - Do not remove this line

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

# {SDK-SETUP:VARS} - Do not remove this line
set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/incubator.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/incubator.ruleset)

# --- Generate EDC from DDL---
process_schema(DDL_FILE ${INCUBATOR_DDL})

# -- Translate ruleset into CPP --
translate_ruleset(RULESET_FILE ${INCUBATOR_RULESET})

add_executable(incubator
    incubator.cpp
)

target_add_gaia_generated_sources(incubator)
add_dependencies(incubator translate_incubator_ruleset)
target_include_directories(incubator PRIVATE ${GAIA_INC})
target_link_directories(incubator PRIVATE ${GAIA_LIB_DIR})
target_link_libraries(incubator PRIVATE gaia rt Threads::Threads)
