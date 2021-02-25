#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set -euo pipefail
IFS=$'\n\t'

DATA_DIR="$1"
PG_CLIENT="$2"
SETUP_SCRIPT="$3"
TEST_SCRIPT="$4"
EXPECTED_FILE="$5"

# export PGHOST=${PGHOST-localhost}
# export PGPORT=${PGPORT-5432}
# export PGDATABASE=${PGDATABASE-postgres}
export PGUSER=${PGUSER-postgres}
# export PGPASSWORD=

# Unzip data files to a folder under /tmp.
TMP_DIR=$(mktemp -d -t airport-data-XXXXXXXXXX)
chmod 755 "$TMP_DIR"
pushd "$DATA_DIR"
for f in *gz; do
    gzip -dkc < $f > "$TMP_DIR/${f%%.gz}"
done
popd

# Set up database tables and import airport data.
"$PG_CLIENT" --set=data_dir="$TMP_DIR" -f "$SETUP_SCRIPT"

# Execute test queries.
OUTPUT=$("$PG_CLIENT" -A -F , -X -t -f "$TEST_SCRIPT")
EXPECTED=$(cat "$EXPECTED_FILE")

# Exit with 0 or 1 depending on comparison result.
[[ "$OUTPUT" = "$EXPECTED" ]]
