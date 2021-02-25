#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set -euo pipefail
IFS=$'\n\t'

PG_CLIENT="$1"
SETUP_SCRIPT="$2"
TEST_SCRIPT="$3"
EXPECTED_FILE="$4"

# export PGHOST=${PGHOST-localhost}
# export PGPORT=${PGPORT-5432}
# export PGDATABASE=${PGDATABASE-postgres}
export PGUSER=${PGUSER-postgres}
# export PGPASSWORD=

# Set up database tables and import airport data.
"$PG_CLIENT" -f "$SETUP_SCRIPT"

# Execute test queries.
OUTPUT=$("$PG_CLIENT" -A -F , -X -t -f "$TEST_SCRIPT")
EXPECTED=$(cat "$EXPECTED_FILE")

# Exit with 0 or 1 depending on comparison result.
[[ "$OUTPUT" = "$EXPECTED" ]]
