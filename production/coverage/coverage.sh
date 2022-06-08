#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

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
    echo "  -o,--output                 Folder to place any output."
    echo "  -s,--shell                  Drop into the bash shell for debugging."
    echo "  -v,--verbose                Show lots of information while executing the project."
    echo "  -h,--help                   Display this help text."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    BASH_MODE=0
    BASE_IMAGE=
    OUTPUT_DIRECTORY=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -s|--shell)
            BASH_MODE=1
            shift
        ;;
        -b|--base-image)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the name of an image." >&2
                show_usage
            fi
            BASE_IMAGE=$2
            shift
            shift
        ;;
        -o|--output)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the directory to place output in." >&2
                show_usage
            fi
            OUTPUT_DIRECTORY=$2
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

    if [ -z "$OUTPUT_DIRECTORY" ] ; then
        complete_process 1 "The -o/--output parameter must be specified."
    fi
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

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Generating the coverage dockerfile."
fi
REPO_ROOT_DIR=$(git rev-parse --show-toplevel)
GDEV_WRAPPER="${REPO_ROOT_DIR}/dev_tools/gdev/gdev.sh"
if ! "${GDEV_WRAPPER}" dockerfile --cfg-enables Coverage > "${REPO_ROOT_DIR}/production/raw_dockerfile" ; then
    cat "${REPO_ROOT_DIR}/production/dockerfile"
    complete_process 1 "Unable to generate raw dockerfile for coverage run."
fi

if ! "$SCRIPTPATH/copy_after_line.py" --input-file "$REPO_ROOT_DIR/production/raw_dockerfile" --output-file "$REPO_ROOT_DIR/production/dockerfile" --start-line "#syntax=docker/dockerfile-upstream:main-experimental" ; then
    complete_process 1 "Unable to generate dockerfile for coverage run."
fi

COVERAGE_IMAGE_BASE=
if [ -n "$BASE_IMAGE" ] ; then
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Basing the coverage image on the '$BASE_IMAGE' image."
    fi
    COVERAGE_IMAGE_BASE="--cache-from $BASE_IMAGE"
fi

echo "--"
echo "before"
du --max-depth=1 -m
df -h
echo "--"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Building the coverage docker image."
fi
echo "docker buildx build \
    -f \"$REPO_ROOT_DIR/production/dockerfile\" \
    -t coverage_image \
    $COVERAGE_IMAGE_BASE \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --platform linux/amd64 \
    --shm-size 1gb \
    --ssh default \
    --compress \
    \"$REPO_ROOT_DIR\""

# shellcheck disable=SC2086
if ! docker buildx build \
    -f "$REPO_ROOT_DIR/production/dockerfile" \
    -t coverage_image \
    $COVERAGE_IMAGE_BASE \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --platform linux/amd64 \
    --shm-size 1gb \
    --ssh default \
    --compress \
    "$REPO_ROOT_DIR" ; then
    complete_process 1 "Coverage build failed."
fi

echo "--"
echo "before"
du --max-depth=1 -m
df -h
echo "--"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Pruning system resources consumed during build process."
fi
if ! docker system prune -f ; then
    complete_process 1 "Unable to prune system resources after image build."
fi

echo "--"
echo "after"
du --max-depth=1 -m
df -h
echo "--"

mkdir -p "$OUTPUT_DIRECTORY"

if [ "$BASH_MODE" -ne 0 ]; then
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing bash in GCov container for debugging."
    fi
    echo "docker run \
        --rm \
        -it \
        --init \
        --platform linux/amd64 \
        --entrypoint /bin/bash \
        --mount type=\"volume,dst=/build/output,volume-driver=local,volume-opt=type=none,volume-opt=o=bind,volume-opt=device=$OUTPUT_DIRECTORY\" \
        coverage_image"

    # shellcheck disable=SC2086
    if ! docker run \
        --rm \
        -it \
        --init \
        --platform linux/amd64 \
        --entrypoint /bin/bash \
        --mount "type=volume,dst=/build/output,volume-driver=local,volume-opt=type=none,volume-opt=o=bind,volume-opt=device=$OUTPUT_DIRECTORY" \
        coverage_image ; then
        complete_process 1 "Coverage run failed."
    fi
else
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing coverage workflow in GCov container."
    fi
    echo "docker run \
        --rm \
        -t \
        --init \
        --platform linux/amd64 \
        --mount type=\"volume,dst=/build/output,volume-driver=local,volume-opt=type=none,volume-opt=o=bind,volume-opt=device=$OUTPUT_DIRECTORY\" \
        coverage_image \
        \"/source/production/coverage/gen_coverage.sh\""

    # shellcheck disable=SC2086
    if ! docker run \
        --rm \
        -t \
        --init \
        --platform linux/amd64 \
        --mount "type=volume,dst=/build/output,volume-driver=local,volume-opt=type=none,volume-opt=o=bind,volume-opt=device=$OUTPUT_DIRECTORY" \
        coverage_image \
        "/source/production/coverage/gen_coverage.sh"; then
        complete_process 1 "Coverage run failed."
    fi
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Switching to the coverage directory."
fi
if ! cd "$SCRIPTPATH" > "$TEMP_FILE" 2>&1; then
    cat "$TEMP_FILE"
    complete_process 1 "Script cannot change to coverage directory before proceeding."
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Setting proper permissions on output directory."
fi
sudo chown -R "$USER":"$USER" "$OUTPUT_DIRECTORY"
sudo chmod -R 777 "$OUTPUT_DIRECTORY"

# If we get here, we have a clean exit from the script.
complete_process 0
