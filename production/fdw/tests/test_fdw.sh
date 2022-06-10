#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

set -euo pipefail
IFS=$'\n\t'

PG_CLIENT=$1
SETUP_SCRIPT=$2
TEST_SCRIPT=$3
EXPECTED_FILE=$4
DATA_DIR=${5:-}

# export PGHOST=${PGHOST-localhost}
# export PGPORT=${PGPORT-5432}
# export PGDATABASE=${PGDATABASE-postgres}
export PGUSER=${PGUSER-postgres}
# export PGPASSWORD=

if [ "$#" == "4" ]; then
    # Set up database tables.
    "$PG_CLIENT" -f "$SETUP_SCRIPT"
else
    # Unzip data files to a folder under /tmp.
    TMP_DIR=$(mktemp -d -t fdw-test-data-XXXXXXXXXX)
    chmod 755 "$TMP_DIR"
    pushd "$DATA_DIR"
    for f in *gz; do
        gzip -dkc < "$f" > "$TMP_DIR/${f%%.gz}"
    done
    popd

    # Set up database tables and import data.
    "$PG_CLIENT" --set=data_dir="$TMP_DIR" -f "$SETUP_SCRIPT"
fi

# Execute test queries.
OUTPUT=$("$PG_CLIENT" -A -F , -X -t -f "$TEST_SCRIPT")
EXPECTED=$(cat "$EXPECTED_FILE")

# Exit with 0 or 1 depending on comparison result.
[[ "$OUTPUT" = "$EXPECTED" ]]
