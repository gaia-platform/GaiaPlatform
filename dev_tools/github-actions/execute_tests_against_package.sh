#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

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
    echo "  -d,--db-persistence Whether the database is started with persistence enabled or disabled."
    echo "  -j,--job-name       GitHub Actions job that this script is being invoked from."
    echo "  -r,--repo-path      Base path of the repository to generate from."
    echo "  -s,--suite-name     Name of the integration test suite to execute."
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
    SUITE_NAME=
    PERSISTENCE_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -d|--db-persistence)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by either 'enabled' or 'disabled'." >&2
                show_usage
            fi
            PERSISTENCE_MODE=$2
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
        -s|--suite-name)
            if [ -z "$2" ] ; then
                echo "Error: Argument $1 must be followed by the integration suite name to execute." >&2
                show_usage
            fi
            SUITE_NAME=$2
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
    if [ "$JOB_NAME" != "Integration_Samples" ] ; then
        if [ -z "$SUITE_NAME" ] ; then
            echo "Error: Argument -s/--suite-name is required for non-Integration_Samples jobs." >&2
            show_usage
        fi
        if [ -n "$PERSISTENCE_MODE" ] ; then
            if [ "$PERSISTENCE_MODE" != "enabled" ] && [ "$PERSISTENCE_MODE" != "disabled" ] ; then
                echo "Error: Argument -d/--db-persistence must be 'enabled' or 'disabled'." >&2
                show_usage
            fi
        fi
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
if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating test output directory."
fi

if ! mkdir -p "$GAIA_REPO/production/tests/results" ; then
    complete_process 1 "Unable to create an output directory for '$JOB_NAME'."
fi

if ! mkdir -p "$GAIA_REPO/production/tests/staging" ; then
    complete_process 1 "Unable to create a staging directory for '$JOB_NAME'."
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Looking for Debian package to install..."
fi
cd "$PACKAGE_PATH" || exit
# shellcheck disable=SC2061
INSTALL_PATH="$(find . -name gaia-*.deb)"
if [[ -z "$INSTALL_PATH" ]] ; then
    ls -la "$PACKAGE_PATH"
    complete_process 1 "Debian package to install could not be found."
fi

## PER JOB CONFIGURATION ##

if [[ "$JOB_NAME" == Integration_Tests* ]] || [[ "$JOB_NAME" == "Performance_Tests" ]] ; then

    cd "$GAIA_REPO/production/tests" || exit

    if ! "$GAIA_REPO/production/tests/reset_database.sh" --verbose --stop --database ; then
        complete_process 1 "Stopping of the database before execution of integration tests failed."
    fi

    PERSISTENCE_FLAG=
    if [ "$PERSISTENCE_MODE" == "enabled" ] ; then
        PERSISTENCE_FLAG="--persistence"
        echo "  using a persistent database."
    else
        echo "  using a non-persistent database."
    fi

    DID_FAIL=0
    if ! ./suite.sh --verbose --json --database $PERSISTENCE_FLAG --memory "$SUITE_NAME" ; then
        DID_FAIL=1
    fi

    cp -a "$GAIA_REPO/production/tests/suite-results" "$GAIA_REPO/production/tests/results"
    if [ $DID_FAIL -ne 0 ] ; then
        complete_process 1 "Tests for job '$JOB_NAME' failed  See job artifacts for more information."
    fi

elif [ "$JOB_NAME" == "Integration_Samples" ] ; then

    # Place the files we need accessible to our docker container into the staging
    # directory.
    cp "${GAIA_REPO}/dev_tools/sdk/test/"* "${GAIA_REPO}/production/tests/staging/"
    cp "${INSTALL_PATH}" "${GAIA_REPO}/production/tests/staging"

    # Only run sample tests instead of the other integration tests
    cd "${GAIA_REPO}/production/tests/staging" || exit

    # Build our empty ubuntu20 container with cmake and clang
    if ! docker buildx build \
        -f Dockerfile_gaia_ubuntu_20 \
        -t gaia_ubuntu_20 \
        --build-arg BUILDKIT_INLINE_CACHE=1 \
        --platform linux/amd64 \
        --shm-size 1gb \
        --ssh default \
        --compress \
        . ; then
        complete_process 1 "Docker build for job failed (gaia_ubuntu_20)."
    fi

    echo "Install path = ${INSTALL_PATH}"

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the Integration Samples tests."
    fi

    # Now build our SDK container which will run the samples script
    if ! docker buildx build \
        -f Dockerfile_gaia_sdk_20 \
        -t gaia_sdk_20 \
        --build-arg BUILDKIT_INLINE_CACHE=1 \
        --build-arg gaia_sdk_deb="${INSTALL_PATH}" \
        --platform linux/amd64 \
        --no-cache \
        --shm-size 1gb \
        --ssh default \
        --progress=plain \
        --compress \
        . ; then
        complete_process 1 "Docker build for job failed (gaia_sdk_20)."
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Integration Sample tests executed successfully."
    fi
fi

## PER JOB CONFIGURATION ##

complete_process 0
