###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(rules)

set(GAIA_RULES_INC "${PROJECT_SOURCE_DIR}/inc")

message(VERBOSE "GAIA_RULES_INC=${GAIA_RULES_INC}")

# Add individual component folders.
add_subdirectory(event_manager)
