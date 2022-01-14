#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing tasks inside of the built Gaia image..."
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
        echo "Executing tasks inside of the Gaia image failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Executing tasks inside of the Gaia image succeeded."
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
    echo "  -g,--gaia-version   Version associate with the build."
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
    GAIA_VERSION=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -j|--job-name)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the name of a job." >&2
                show_usage
            fi
            JOB_NAME=$2
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

    if [ -z "$JOB_NAME" ] ; then
        echo "Error: Argument -j/--job-name is required" >&2
        show_usage
    fi
    if [ -z "$GAIA_VERSION" ] ; then
        echo "Error: Argument -g/--gaia-version is required" >&2
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
TEMP_FILE=$(mktemp /tmp/post_build_inside.XXXXXXXXX)

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory

cd /build/production || exit

mkdir -p /build/output
cp /build/production/*.log /build/output

## PER JOB CONFIGURATION ##

if [ "$JOB_NAME" == "Core" ] ; then
    if ! ctest 2>&1 | tee /build/output/ctest.log; then
        complete_process 1 "Unit tests failed to complete successfully."
    fi

elif [ "$JOB_NAME" == "SDK" ] ; then
    #cp gaia-${{ env.GAIA_VERSION }}_amd64.deb gaia-${{ env.GAIA_VERSION }}-${{ github.run_id }}_amd64.deb
    mkdir -p /build/output/package
    cp /build/production/"gaia-${GAIA_VERSION}_amd64.deb" "/build/output/package/gaia-${GAIA_VERSION}_amd64.deb"

fi

## PER JOB CONFIGURATION ##

complete_process 0

