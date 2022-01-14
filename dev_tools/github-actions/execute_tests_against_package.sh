#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Running tests against the provided docker Debian package..."
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
        echo "Running the tests against the package failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Running the tests against the package succeeded."
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
    echo "  -j,--job-name       GitHub Actions job that this script is being invoked from."
    echo "  -r,--repo-path      Base path of the repository to generate from."
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
    PACKAGE_PATH=
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
        -p|--package)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the path to the package." >&2
                show_usage
            fi
            PACKAGE_PATH=$2
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
    if [ -z "$PACKAGE_PATH" ] ; then
        echo "Error: Argument -p/--package is required" >&2
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
TEMP_FILE=$(mktemp /tmp/execute_tests_against_package.XXXXXXXXX)

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory

# Ensure we have a predicatable place to place output that we want to expose.
if ! mkdir -p "$GAIA_REPO/production/tests/results" ; then
    complete_process 1 "Unable to create an output directory for '$JOB_NAME'."
fi

cd "$PACKAGE_PATH" || exit
# shellcheck disable=SC2061
sudo apt --assume-yes install "$(find . -name gaia*)"

## PER JOB CONFIGURATION ##

if [ "$JOB_NAME" == "Integration_Smoke" ] ; then

    sudo "$GAIA_REPO/production/tests/reset_database.sh" --verbose --stop --database

    DID_FAIL=0
    if ! "$GAIA_REPO/production/tests/smoke_suites.sh" --verbose ; then
        DID_FAIL=1
    fi
    cp -a "$GAIA_REPO/production/tests/suites" "$GAIA_REPO/production/tests/results"
    if [ $DID_FAIL -ne 0 ] ; then
        complete_process 1 "Tests for job '$JOB_NAME' failed  See job artifacts for more information."
    fi

elif [ "$JOB_NAME" == "Integration_Smoke_Persistence" ] ; then

    sudo "$GAIA_REPO/production/tests/reset_database.sh" --verbose --stop --database

    DID_FAIL=0
    if ! "$GAIA_REPO/production/tests/smoke_suites_with_persistence.sh" --verbose ; then
        DID_FAIL=1
    fi
    cp -a "$GAIA_REPO/production/tests/suites" "$GAIA_REPO/production/tests/results"
    if [ $DID_FAIL -ne 0 ] ; then
        complete_process 1 "Tests for job '$JOB_NAME' failed  See job artifacts for more information."
    fi

elif [ "$JOB_NAME" == "Integration_Samples" ] ; then

    cd "$GAIA_REPO/dev_tools/sdk/test" || exit
    if ! sudo bash -c "./build_sdk_samples.sh > \"$GAIA_REPO/production/tests/results/test.log\"" ; then
        complete_process 1 "Tests for job '$JOB_NAME' failed  See job artifacts for more information."
    fi
fi

## PER JOB CONFIGURATION ##

complete_process 0

