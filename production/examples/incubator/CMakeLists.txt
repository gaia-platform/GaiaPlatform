#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# {SDK-SETUP:CMAKE} - Do not remove this line

# Set the project name.
project(incubator)

# {SDK-SETUP:CXX_STANDARD} - Do not remove this line
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")

# Enable AddressSanitizer in debug mode.
if(ENABLE_ASAN AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address")
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
target_link_libraries(incubator PRIVATE gaia rt pthread)
