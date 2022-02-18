#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Note that this script makes use of the following resources:
#
# - https://chromium.googlesource.com/chromium/src.git/+/65.0.3283.0/docs/ios/coverage.md
# - https://llvm.org/docs/CommandGuide/llvm-cov.html#llvm-cov-show
# - https://clang.llvm.org/docs/SourceBasedCodeCoverage.html

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
    echo "  -q,--quiet                  Do not show lots of information while executing the project."
    echo "  -h,--help                   Display this help text."
    echo ""
    show_usage_commands
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=1
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -q|--quiet)
            VERBOSE_MODE=0
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

export COVERAGE_PROFILE="./tests.profdata"
export COVERAGE_BINARIES="catalog/gaiac/gaiac -object db/core/gaia_db_server -object tools/gaia_translate/gaiat tools/gaia_db_extract"
export COVERAGE_HTML_FLAGS="-format=html -show-expansions -show-regions"
export COVERAGE_EXPORT_FLAGS="-format=lcov"

# Generate a report for a given subset of the source tree.
#
# Note that SC2086 is disabled a lot because it is replacing multiple values.
#
# For more information:
#   https://www.shellcheck.net/wiki/SC2086 -- Double quote to prevent globbing ...
generate_report() {
    local COVERAGE_TYPE=$1
    local HTML_OUTPUT_DIR=$2
    local FILTER_NAME=$3
    local SOURCE_DIRECTORIES=$4

    echo "Creating HTML representation of coverage $COVERAGE_TYPE."
    mkdir -p "$HTML_OUTPUT_DIR"
    # shellcheck disable=SC2086
    if ! /usr/lib/llvm-13/bin/llvm-cov show \
        -instr-profile=$COVERAGE_PROFILE \
        -output-dir="$HTML_OUTPUT_DIR" \
        $COVERAGE_HTML_FLAGS \
        $COVERAGE_BINARIES \
        $SOURCE_DIRECTORIES ; then
        complete_process 1 "Unable to create HTML representation of coverage $COVERAGE_TYPE."
    fi
    chmod -R 666 "$HTML_OUTPUT_DIR/coverage"
    chmod 666 "$HTML_OUTPUT_DIR/index.html"
    chmod 666 "$HTML_OUTPUT_DIR/style.css"

    echo "Generating export for $COVERAGE_TYPE."
    # shellcheck disable=SC2086
    if ! /usr/lib/llvm-13/bin/llvm-cov export \
        -instr-profile=$COVERAGE_PROFILE \
        $COVERAGE_EXPORT_FLAGS \
        $COVERAGE_BINARIES \
        $SOURCE_DIRECTORIES > "$FILTER_NAME" ; then
        complete_process 1 "Unable to generate export for $COVERAGE_TYPE."
    fi
}

# Set up any project based local script variables.
TEMP_FILE=/tmp/blah.tmp
DID_PUSHD=0

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process

echo "Downloading LLVM-13."
wget --no-check-certificate -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
add-apt-repository 'deb http://apt.llvm.org/focal/   llvm-toolchain-focal-13  main'
apt update
apt-get install -y llvm-13 lldb-13 llvm-13-dev libllvm13 llvm-13-runtime lcov

echo "Installing Python helper packages."
pip install lcov_cobertura
chmod +x /usr/local/lib/python3.8/dist-packages/lcov_cobertura.py

echo "Cleaning the output directory."
mkdir -p /build/production/output
pushd /build/production/output || exit
rm -rf ./*
popd || exit

# Note that for the purposes of coverage, we are not concerned with pass/fail.
# That responsibility is covered by the other jobs.
echo "Running tests with profile-enabled binaries."
export LLVM_PROFILE_FILE="/build/production/tests.%4m.profraw"

ctest --output-log /build/production/output/ctest.log --output-junit /build/production/output/ctest.xml
/source/production/tests/llvm.sh --database --verbose

echo "Creating an input file of all created profiles."
find . -name "*.profraw" ! -path "./llvm/*" > /build/production/output/profiles.txt
cat /build/production/output/profiles.txt

echo "Merging all profiles into a single profile data file."
if ! /usr/lib/llvm-13/bin/llvm-profdata merge --input-files /build/production/output/profiles.txt --output tests.profdata ; then
    complete_process 1 "Unable to merge profiles into a single data file."
fi

#
# Source directories, by team:
#
# Rules:
# - direct_access
# - tools
#
# Database:
# - catalog
# - db
#
# Common (needs further refining):
# - inc
# - common
#
echo "Generate the various flavors of reports."
generate_report \
    "total" \
    "/build/production/output/total" \
    "/build/production/output/coverage.filter" \
    "/source/production"

generate_report \
    "rules" \
    "/build/production/output/rules" \
    "/build/production/output/coverage.rules" \
    "/source/production/direct_access /source/production/tools"

generate_report \
    "database" \
    "/build/production/output/database" \
    "/build/production/output/coverage.database" \
    "/source/production/db /source/production/catalog"

generate_report \
    "common" \
    "/build/production/output/common" \
    "/build/production/output/coverage.common" \
    "/source/production/inc /source/production/common"

echo "Producing Cobertura Coverage files."
if ! /usr/local/lib/python3.8/dist-packages/lcov_cobertura.py /build/production/output/coverage.filter \
    --base-dir /source/production \
    --output /build/production/output/coverage.xml ; then
    complete_process 1 "Unable to produce Cobertura Coverage file: coverage.xml"
fi
if ! /usr/local/lib/python3.8/dist-packages/lcov_cobertura.py /build/production/output/coverage.rules \
    --base-dir /source/production \
    --output /build/production/output/rules.xml ; then
    complete_process 1 "Unable to produce Cobertura Coverage file: rules.xml"
fi
if ! /usr/local/lib/python3.8/dist-packages/lcov_cobertura.py /build/production/output/coverage.database \
    --base-dir /source/production \
    --output /build/production/output/database.xml ; then
    complete_process 1 "Unable to produce Cobertura Coverage file: database.xml"
fi
if ! /usr/local/lib/python3.8/dist-packages/lcov_cobertura.py /build/production/output/coverage.common \
    --base-dir /source/production \
    --output /build/production/output/common.xml ; then
    complete_process 1 "Unable to produce Cobertura Coverage file: common.xml"
fi

mkdir -p /build/output
cp -a /build/production/output /build

# If we get here, we have a clean exit from the script.
complete_process 0
