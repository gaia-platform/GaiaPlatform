#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# shellcheck disable=SC2128
if [ "$0" = "$BASH_SOURCE" ]; then
    echo "WARNING"
    echo "This script is not meant to be executed directly!"
    echo "Use this script only by sourcing it and using the 'execute_suites' entry point."
    exit 1
fi

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the smoke test suites..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    COMPLETE_MESSAGE=
    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        COMPLETE_MESSAGE="Execution of the smoke test suites failed."
        echo "$COMPLETE_MESSAGE"
    else
        COMPLETE_MESSAGE="Execution of the smoke test suites succeeded."
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "$COMPLETE_MESSAGE"
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
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
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

# Save the current directory when starting the script, so we can go back to that
# directory at the end of the script.
save_current_directory() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Saving current directory prior to test execution."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Suite script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Clear the output directory so we have a clean place to store the suite results into.
clear_suite_output() {
    if [ "$SUITES_DIRECTORY" == "" ]; then
        complete_process 1 "Removing the specified directory '$SUITES_DIRECTORY' is dangerous. Aborting."
    fi

    if [ -d "$SCRIPTPATH/$SUITES_DIRECTORY" ]; then
        if [ "$(ls -A "$SCRIPTPATH/$SUITES_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$SCRIPTPATH/$SUITES_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                complete_process 1 "Suite script cannot remove suites directory '$(realpath "$SCRIPTPATH/$SUITES_DIRECTORY")' prior to execution."
            fi
        fi
    else
        if ! mkdir "$SCRIPTPATH/$SUITES_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Suite script cannot create suites directory '$(realpath "$SCRIPTPATH/$SUITES_DIRECTORY")' prior to execution."
        fi
    fi
}

execute_suites() {

    # shellcheck disable=SC2164
    SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

    if [ -z "$SUITE_MODE" ] ; then
        echo "Environment variable 'SUITE_MODE' must be set prior to calling this function."
        exit 1
    fi
    if [ -z "$TEST_WORKLOADS" ] ; then
        echo "Environment variable 'TEST_WORKLOADS' must be set prior to calling this function."
        exit 1
    fi
    if [ -z "$USE_PERSISTENT_DATABASE" ] ; then
        echo "Environment variable 'USE_PERSISTENT_DATABASE' must be set prior to calling this function."
        exit 1
    fi
    if [ -z "$USE_MEMORY_SAMPLING" ] ; then
        echo "Environment variable 'USE_MEMORY_SAMPLING' must be set prior to calling this function."
        exit 1
    fi

    # Set up any project based local script variables.
    TEMP_FILE=/tmp/$SUITE_MODE.suite.tmp
    SUITES_DIRECTORY=suites
    SUITE_RESULTS_DIRECTORY="suite-results"

    # Set up any local script variables.
    DID_PUSHD=0

    # Parse any command line values.
    parse_command_line "$@"

    # Reset this after the command line, now that we know what the selected
    # SUITE_MODE is.
    TEMP_FILE=/tmp/$SUITE_MODE.smoke-suite.tmp

    # Clean entrance into the script.
    start_process

    save_current_directory

    cd "$SCRIPTPATH" || exit

    clear_suite_output

    WORKLOAD_INDEX=0
    DID_FAIL=0
    while [ $WORKLOAD_INDEX -lt ${#TEST_WORKLOADS[@]} ]
    do
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Executing workload: ${TEST_WORKLOADS[$WORKLOAD_INDEX]}"
        fi

        if [ "$USE_PERSISTENT_DATABASE" -ne 0 ] ; then
            PERSISTENCE_FLAG="--persistence"
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "  using a persistent database."
            fi
        else
            PERSISTENCE_FLAG=
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "  using a non-persistent database."
            fi
        fi
        MEMORY_FLAG=
        if [ "$USE_MEMORY_SAMPLING" -ne 0 ] ; then
            MEMORY_FLAG="--memory"
        fi
        # shellcheck disable=SC2086
        if ! "$SCRIPTPATH/suite.sh" --verbose --json --database $PERSISTENCE_FLAG $MEMORY_FLAG "${TEST_WORKLOADS[$WORKLOAD_INDEX]}" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            DID_FAIL=1
        fi

        if ! cp -r "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY" "$SCRIPTPATH/$SUITES_DIRECTORY/${TEST_WORKLOADS[$WORKLOAD_INDEX]}" ; then
            complete_process 1 "Unable to copy the results of the ${TEST_WORKLOADS[$WORKLOAD_INDEX]} suite.  Stopping due to critical error."
        fi

        (( WORKLOAD_INDEX++ )) || true
    done

    if [ $DID_FAIL -ne 0 ] ; then
        complete_process 1 "One or more of the test suites failed to execute."
    fi

    if ! "$SCRIPTPATH/python/summarize_suites.py" --directory "$SCRIPTPATH/suites" --output "$SCRIPTPATH/suites/suites-summary.json" ; then
        complete_process 1 "All tests executed, but one or more of the test suites failed."
    fi

    # If we get here, we have a clean exit from the script.
    complete_process 0
}
