#!/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Generating coverage reports..."
    fi

    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1

    if ! cd /build/production >"$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot change to build directory before proceeding."
    fi
}

# Simple function to stop the process, including any cleanup.
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Generation of coverage reports failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Generation of coverage reports succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    exit "$SCRIPT_RETURN_CODE"
}

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags] <command>"
    echo "Flags:"
    echo "  -v,--verbose                Show lots of information while executing the project."
    echo "  -h,--help                   Display this help text."
    echo ""
    show_usage_commands
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -v|--verbose)
            VERBOSE_MODE=1
            shift
        ;;
        -h|--help)
            show_usage
        ;;
        -*) # unsupported flags
            echo "Error: Unsupported flag $1" >&2
            show_usage
        ;;
        *) # preserve positional arguments
            PARAMS+=("$1")
            shift
        ;;
    esac
    done
}

# Set up any global script variables.

# Set up any project based local script variables.
TEMP_FILE=/tmp/blah.tmp
DID_PUSHD=0

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process

PACKAGES=(
    # We need this for llvm-cov.
    llvm-10
    lcov
    nano
    zip
    unzip
)
# shellcheck disable=SC2145
echo "Installing additional packages: ${PACKAGES[@]}"
apt -y update
# shellcheck disable=SC2068
apt-get install -y ${PACKAGES[@]}

pushd /build/production/output || exit
rm -rf ./*
popd || exit

echo "Setting baseline state"
lcov \
    --no-external \
    --config-file /source/production/coverage/.lcovrc \
    --capture \
    --initial \
    --directory /build/production \
    --base-directory /source/production \
    --gcov-tool /source/production/coverage/llvm-gcov.sh \
    -o /build/production/coverage.base \
    > /build/production/output/base.log
echo "Running tests"
ctest > /build/production/output/ctest.log
echo "Capturing current state"
lcov \
    --no-external \
    --config-file /source/production/coverage/.lcovrc \
    --capture \
    --directory /build/production \
    --base-directory /source/production \
    --gcov-tool /source/production/coverage/llvm-gcov.sh \
    -o /build/production/coverage.test \
    > /build/production/output/test.log
echo "Calculating difference between baselines and current"
lcov \
    --config-file /source/production/coverage/.lcovrc \
    -a /build/production/coverage.base \
    -a /build/production/coverage.test \
    -o /build/production/coverage.total \
    > /build/production/output/total.log
echo "Filtering for total..."
lcov \
    --config-file /source/production/coverage/.lcovrc \
    -r /build/production/coverage.total \
    "/build/production/generated/*" \
    "/build/production/*/*" \
    "/source/production/generated/*" \
    "/source/production/parser/generated/*" \
    "/source/production/catalog/parser/tests/*" \
    "/source/production/catalog/tests/*" \
    "/source/production/common/tests/*" \
    "/source/production/db/core/tests/*" \
    "/source/production/db/index/tests/*" \
    "/source/production/db/memory_manager/tests/*" \
    "/source/production/db/payload_types/tests/*" \
    "/source/production/db/query_processor/tests/*" \
    "/source/production/direct_access/tests/*" \
    "/source/production/rules/event_manager/tests/*" \
    "/source/production/sdk/tests/*" \
    "/source/production/system/tests/*" \
    "/source/production/tools/gaia_dump/tests/*" \
    -o /build/production/coverage.filter \
    > /build/production/output/filter.log

echo "Filtering for rules..."
echo "- /source/production/direct_access"
echo "- /source/production/rules"
echo "- /source/production/system"
lcov \
    --config-file /source/production/coverage/.lcovrc \
    -r /build/production/coverage.filter \
    "/source/production/catalog/*" \
    "/source/production/common/*" \
    "/source/production/db/*" \
    "/source/production/inc/*" \
    "/source/production/schemas/*" \
    "/source/production/sql/*" \
    "/source/production/tools/*" \
    -o /build/production/coverage.rules \
    > /build/production/output/rules.log

echo "Filtering for database..."
echo "- /source/production/catalog"
echo "- /source/production/db"
echo "- /source/production/sql"
lcov \
    --config-file /source/production/coverage/.lcovrc \
    -r /build/production/coverage.filter \
    "/source/production/common/*" \
    "/source/production/direct_access/*" \
    "/source/production/inc/*" \
    "/source/production/rules/*" \
    "/source/production/schemas/*" \
    "/source/production/system/*" \
    "/source/production/tools/*" \
    -o /build/production/coverage.database \
    > /build/production/output/database.log

echo "Filtering for other..."
echo "- /source/production/common"
echo "- /source/production/inc"
echo "- /source/production/schemas"
echo "- /source/production/tools"
lcov \
    --config-file /source/production/coverage/.lcovrc \
    -r /build/production/coverage.filter \
    "/source/production/catalog/*" \
    "/source/production/db/*" \
    "/source/production/direct_access/*" \
    "/source/production/rules/*" \
    "/source/production/sql/*" \
    "/source/production/system/*" \
    -o /build/production/coverage.other \
    > /build/production/output/other.log

echo "Producing total HTML files."
mkdir /build/production/output/total
genhtml \
    --config-file /source/production/coverage/.lcovrc \
    --ignore-errors source \
    -p /build/production \
    --legend \
    --demangle-cpp \
    coverage.filter \
    -o /build/production/output/total \
    > gen-total.log

echo "Producing Database HTML files."
mkdir /build/production/output/database
genhtml \
    --config-file /source/production/coverage/.lcovrc \
    --ignore-errors source \
    -p /build/production \
    --legend \
    --demangle-cpp \
    coverage.database \
    -o /build/production/output/database \
    > gen-database.log

echo "Producing Rules HTML files."
mkdir /build/production/output/rules
genhtml \
    --config-file /source/production/coverage/.lcovrc \
    --ignore-errors source \
    -p /build/production \
    --legend \
    --demangle-cpp \
    coverage.rules \
    -o /build/production/output/rules \
    > gen-rules.log

echo "Producing Other HTML files."
mkdir /build/production/output/other
genhtml \
    --config-file /source/production/coverage/.lcovrc \
    --ignore-errors source \
    -p /build/production \
    --legend \
    --demangle-cpp \
    coverage.other \
    -o /build/production/output/other \
    > gen-other.log

echo "Producing RAW HTML files."
mkdir /build/production/output/raw
genhtml \
    --config-file /source/production/coverage/.lcovrc \
    --ignore-errors source \
    --legend \
    --demangle-cpp \
    coverage.total \
    -o /build/production/output/raw \
    > gen-raw.log

# If we get here, we have a clean exit from the script.
complete_process 0
