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

    if ! cd $SCRIPTPATH >"$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot change to coverage directory before proceeding."
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
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=/tmp/blah.tmp
DID_PUSHD=0

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process


if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating/cleaning output directory."
fi
mkdir output > "$TEMP_FILE" 2>&1
if ! rm -rf "$SCRIPTPATH/output"/* > "$TEMP_FILE" 2>&1; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot clean output directory before proceeding."
fi


if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Placing tooled 'gdev.cfg' file in root production directory."
fi
if ! diff "$SCRIPTPATH/../gdev.cfg" "$SCRIPTPATH/gdev.cfg" > "$TEMP_FILE" 2>&1; then
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Backing up existing gdev.cfg in the root production directory."
    fi
    cp "$SCRIPTPATH/../gdev.cfg" "$SCRIPTPATH/../gdev.cfg.old"
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Copying coverage gdev.cfg to the root production directory."
    fi
    cp "$SCRIPTPATH/gdev.cfg" "$SCRIPTPATH/../gdev.cfg"
else
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Coverage gdev.cfg is already present in the root production directory."
    fi
fi


if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Switching to the root production directory."
fi
if ! cd "$SCRIPTPATH/.." > "$TEMP_FILE" 2>&1; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot change to root production directory before proceeding."
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Executing coverage workflow in GCov container."
fi

gdev run --mounts ./coverage/output:output /source/production/coverage/gen_coverage.sh

if [[ -f $SCRIPTPATH/../gdev.cfg.old ]]; then
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Restoring previous gdev.cfg file to root production directory."
    fi
    cp "$SCRIPTPATH/../gdev.cfg.old" "$SCRIPTPATH/../gdev.cfg"
    rm "$SCRIPTPATH/../gdev.cfg.old"
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Setting proper permissions on output directory."
fi
sudo chown -R "$USER":"$USER" "$SCRIPTPATH/output"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Switching to the coverage directory."
fi
if ! cd "$SCRIPTPATH" > "$TEMP_FILE" 2>&1; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot change to coverage directory before proceeding."
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating coverage.json file from converage output."
fi
./summarize.py > "$SCRIPTPATH/output/coverage.json"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating coverage.zip file from converage output."
fi
zip -r "$SCRIPTPATH/output/coverage.zip" "$SCRIPTPATH/output"

# If we get here, we have a clean exit from the script.
complete_process 0
