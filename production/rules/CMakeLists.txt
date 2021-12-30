#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(rules)

set(GAIA_RULES_INC "${PROJECT_SOURCE_DIR}/inc")

message(VERBOSE "GAIA_RULES_INC=${GAIA_RULES_INC}")

# Add individual component folders.
add_subdirectory(event_manager)
