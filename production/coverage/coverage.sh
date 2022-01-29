#!/usr/bin/env bash

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

    if ! cd "$SCRIPTPATH" >"$TEMP_FILE" 2>&1; then
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
    echo "  -b,--bash                   Drop into bash for debugging."
    echo "  -v,--verbose                Show lots of information while executing the project."
    echo "  -h,--help                   Display this help text."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    BASH_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -b|--bash)
            BASH_MODE=1
            shift
        ;;
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
mkdir -p "$SCRIPTPATH/output" > "$TEMP_FILE" 2>&1
if ! rm -rf "$SCRIPTPATH/output"/* > "$TEMP_FILE" 2>&1; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot clean output directory before proceeding."
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Switching to the root production directory."
fi
if ! cd "$SCRIPTPATH/.." > "$TEMP_FILE" 2>&1; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot change to root production directory before proceeding."
fi

if [ "$BASH_MODE" -ne 0 ]; then
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing bash in GCov container for debugging."
    fi
    CONTAINER_SCRIPT_TO_RUN=
else
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing coverage workflow in GCov container."
    fi
    CONTAINER_SCRIPT_TO_RUN=/source/production/coverage/gen_coverage.sh
fi

REPO_ROOT_DIR=$(git rev-parse --show-toplevel)
GDEV_WRAPPER="${REPO_ROOT_DIR}/dev_tools/gdev/gdev.sh"
if ! "${GDEV_WRAPPER}" run --cfg-enables Coverage \
    $CONTAINER_SCRIPT_TO_RUN ; then
    complete_process 1 "Unable to execute a coverage run inside of the Docker container."
fi
#     --mounts "$SCRIPTPATH/output:/build/production/output" \

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
    echo "Creating coverage-summary.json file from coverage output."
fi
if ! /usr/bin/python3.8 "$SCRIPTPATH/summarize.py" > "$TEMP_FILE" 2>&1 ; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot summarize coverage directory after proceeding."
fi

# If we get here, we have a clean exit from the script.
complete_process 0
