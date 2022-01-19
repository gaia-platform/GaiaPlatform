#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the post build action..."
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
        echo "Executing the post build action failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Executing the post build action succeeded."
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
    echo "  -a,--action         Action to execute inside of the container."
    echo "  -g,--gaia-version   Version associate with the build."
    echo "  -r,--repo-path      Base path of the repository to generate from."
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    ACTION_NAME=
    GAIA_REPO=
    GAIA_VERSION=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -a|--action)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the name of the action." >&2
                show_usage
            fi
            ACTION_NAME=$2
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
        -g|--gaia-version)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the version of Gaia being built." >&2
                show_usage
            fi
            GAIA_VERSION=$2
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
    if [ -z "$GAIA_VERSION" ] ; then
        echo "Error: Argument -g/--gaia-version is required" >&2
        show_usage
    fi
    if [ -z "$ACTION_NAME" ] ; then
        echo "Error: Argument -a/--action is required" >&2
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

cd "$GAIA_REPO/production" || exit

# Ensure we have a predicatable place to place output that we want to expose.
if ! mkdir -p "$GAIA_REPO/build/output" ; then
    complete_process 1 "Unable to create an output directory to capture the results."
fi

if ! docker run \
    --rm \
    --init \
    -t \
    --platform linux/amd64 \
    --mount "type=volume,dst=/build/output,volume-driver=local,volume-opt=type=none,volume-opt=o=bind,volume-opt=device=$GAIA_REPO/build/output" \
    build_image \
    /source/dev_tools/github-actions/post_build_inside_container.sh --action "$ACTION_NAME" --gaia-version "$GAIA_VERSION" ; then
    complete_process 1 "Docker post-build script failed."
fi

complete_process 0

