#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(db)

set(GAIA_DB_INC "${PROJECT_SOURCE_DIR}/inc")

message(VERBOSE "GAIA_DB_INC=${GAIA_DB_INC}")

# Add individual component folders.
add_subdirectory(memory_manager)
add_subdirectory(core)
add_subdirectory(payload_types)
add_subdirectory(index)
add_subdirectory(triggers)
add_subdirectory(query_processor)
