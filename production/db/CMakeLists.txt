###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(db)

set(GAIA_DB_INC "${PROJECT_SOURCE_DIR}/inc")

message(VERBOSE "GAIA_DB_INC=${GAIA_DB_INC}")

# Add individual component folders.
add_subdirectory(core)
add_subdirectory(payload_types)
add_subdirectory(index)
add_subdirectory(triggers)
add_subdirectory(query_processor)
