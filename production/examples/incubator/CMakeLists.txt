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
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -O3")
set(GAIA_LINK_FLAGS " ")

# Debug build-specific options.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -O0 -g -fno-limit-debug-info")
  if(ENABLE_ASAN)
    set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer")
    set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all")
  endif()
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
set_target_properties(incubator PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(incubator PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_include_directories(incubator PRIVATE ${GAIA_INC})
target_link_libraries(incubator PRIVATE gaia rt Threads::Threads)
