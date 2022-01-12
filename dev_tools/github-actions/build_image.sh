#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Building the docker image..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Building the docker image failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Building the docker image succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    # Restore the current directory.
    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi

    exit "$SCRIPT_RETURN_CODE"
}

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags]"
    echo "Flags:"
    echo "  -r,--repo-path      Base path of the repository to generate from."
    echo "  -j,--job-name       GitHub Actions job that this script is being invoked from."
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    JOB_NAME=
    GAIA_REPO=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -r|--repo-path)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the path to the repository." >&2
                show_usage
            fi
            GAIA_REPO=$2
            shift
            shift
        ;;
        -j|--job-name)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the name of a job." >&2
                show_usage
            fi
            JOB_NAME=$2
            shift
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

    if [ -z "$GAIA_REPO" ] ; then
        echo "Error: Argument -r/--repo-path is required" >&2
        show_usage
    fi
    if [ -z "$JOB_NAME" ] ; then
        echo "Error: Argument -j/--job-name is required" >&2
        show_usage
    fi
}

# Save the current directory when starting the script, so we can go back to that
# directory at the end of the script.
save_current_directory() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Saving current directory prior to linting."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Lint script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Set up any global script variables.
# shellcheck disable=SC2164
# SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=$(mktemp /tmp/build_image.XXXXXXXXX)

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory

CONFIGURATION_OPTIONS=
if [ "$JOB_NAME" == "Core" ] ; then
    CONFIGURATION_OPTIONS=--cfg-enables ubuntu:20.04
else
    complete_process 1 "Cannot build docker image for job named '$JOB_NAME'."
fi

mkdir -p "$GAIA_REPO/build/output"

pip install atools argcomplete

"$GAIA_REPO/dev_tools/gdev/gdev.sh" dockerfile > "$GAIA_REPO/production/dockerfile"
cp "$GAIA_REPO/production/dockerfile" "$GAIA_REPO/build/output"

docker buildx build \
    -f "$GAIA_REPO/production/dockerfile" \
    -t build_image \
    $CONFIGURATION_OPTIONS \
    --cache-from ghcr.io/gaia-platform/dev-base:latest \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --platform linux/amd64 \
    --shm-size 1gb \
    --ssh default \
    --compress \
    "$GAIA_REPO/production"

complete_process 0

