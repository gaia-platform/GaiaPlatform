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
    echo "  -b,--base-image     Optional base image to use as a cache for the new image."
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
    BASE_IMAGE=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -b|--base-image)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the name of an image." >&2
                show_usage
            fi
            BASE_IMAGE=$2
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
        -r|--repo-path)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the path to the repository." >&2
                show_usage
            fi
            GAIA_REPO=$2
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
        echo "Saving current directory prior to execution."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Set up any project based local script variables.
TEMP_FILE=$(mktemp /tmp/build_image.XXXXXXXXX)

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory

## PER JOB CONFIGURATION ##

CONFIGURATION_OPTIONS=
if [ "$JOB_NAME" == "Core" ] ; then
    CONFIGURATION_OPTIONS="--cfg-enables CI_GitHub"
elif [ "$JOB_NAME" == "Debug_Core" ] ; then
    CONFIGURATION_OPTIONS="--cfg-enables CI_GitHub --cfg-enables Debug"
elif [ "$JOB_NAME" == "SDK" ] ; then
    CONFIGURATION_OPTIONS="--cfg-enables CI_GitHub --cfg-enables GaiaSDK"
elif [ "$JOB_NAME" == "LLVM_Tests" ] ; then
    CONFIGURATION_OPTIONS="--cfg-enables CI_GitHub --cfg-enables GaiaLLVMTests"
else
    complete_process 1 "Cannot build docker image for job named '$JOB_NAME'."
fi

## PER JOB CONFIGURATION ##

# Ensure we have a predicatable place to place output that we want to expose.
if ! mkdir -p "$GAIA_REPO/build/output" ; then
    complete_process 1 "Unable to create an output directory for '$JOB_NAME'."
fi

# Execute GDev to produce a dockerfile with the specified configuration options.
cd "$GAIA_REPO/production" || exit
if ! pip install atools argcomplete ; then
    complete_process 1 "Unable to install Python packages required to build dockerfile for '$JOB_NAME'."
fi
# shellcheck disable=SC2086
if ! "$GAIA_REPO/dev_tools/gdev/gdev.sh" dockerfile $CONFIGURATION_OPTIONS > "$GAIA_REPO/production/dockerfile" ; then
    complete_process 1 "Creation of dockerfile for job '$JOB_NAME' failed."
fi

# Copy that dockerfile to our output directory for later debugging, if needed.
if ! cp "$GAIA_REPO/production/dockerfile" "$GAIA_REPO/build/output" ; then
    complete_process 1 "Copy of dockerfile for job '$JOB_NAME' failed."
fi

# Execute `docker buildx build` to create a dockerfile.
# Note that using `buildx` will use multiple cores for the build, where possible.
if [ -n "$BASE_IMAGE" ] ; then
    BASE_IMAGE="--cache-from $BASE_IMAGE"
fi

# shellcheck disable=SC2086
if ! docker buildx build \
    -f "$GAIA_REPO/production/dockerfile" \
    -t build_image \
    $BASE_IMAGE \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --platform linux/amd64 \
    --shm-size 1gb \
    --ssh default \
    --compress \
    "$GAIA_REPO" ; then
    complete_process 1 "Docker build for job '$JOB_NAME' failed."
fi

complete_process 0

