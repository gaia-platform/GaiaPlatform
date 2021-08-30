#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags] [test-name]"
    echo "Flags:"
    echo "  -d,--directory              Directory to install the tests to and execute from."
    echo "  -ni,--no-init               Do not initialize the test data before executing the test."
    echo "  -nt,--num-threads <threads> Number of threads to use for the rule engine.  If '0' is"
    echo "                              specified, then the number of threads is set to maximum."
    echo "  -vv,--very-verbose          Verbose for this script and any top level scripts it calls."
    echo "  -v,--verbose                Display detailed information during execution."
    echo "  -h,--help                   Display this help text."
    echo "Arguments:"
    echo "  test-name                   Optional name of the test to run.  (Default: 'smoke')"
    exit 1
}

# Parse the command line.
parse_command_line() {
    TEST_MODE="smoke"
    VERBOSE_MODE=0
    NO_INIT_MODE=0
    NUMBER_OF_THREADS=-1
    TEST_DIRECTORY=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -vv|--very-verbose)
            VERBOSE_MODE=1
            shift
        ;;
        -ni|--no-init)
            NO_INIT_MODE=1
            shift
        ;;
        -d|--directory)
            # shellcheck disable=SC2086
            if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
                TEST_DIRECTORY=$2
                shift 2
            else
                echo "Error: Argument for $1 is missing." >&2; exit 1
            fi
        ;;
        -nt|--num-threads)
            # shellcheck disable=SC2086
            if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
                regex_to_match='^[0-9]+$'
                if ! [[ $2 =~ $regex_to_match ]] ; then
                    echo "Error: Argument for $1 is not a non-negative integer." >&2; exit 1
                fi
                NUMBER_OF_THREADS=$2
                shift 2
            else
                echo "Error: Argument for $1 is missing." >&2; exit 1
            fi
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

    if [[ ! "${PARAMS[0]}" == "" ]]; then
        TEST_MODE=${PARAMS[0]}
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    # Restore the current directory.
    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi

    exit "$SCRIPT_RETURN_CODE"
}


save_current_directory() {
    # Save the current directory so we can get back to it.
    if ! pushd . > "$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        echo "Test script cannot save the current directory before proceeding with test execution."
        complete_process 1
    fi
    DID_PUSHD=1
}

create_results_directory() {
    # Yes, this is overkill, but when using a rm -rf, cannot be too careful.
    #
    # This is overkill because doing a 'rm -rf /*' with sudo permissions will attempt
    # to remove every file in the system or at the very least, all files in the directory.
    if [ -d "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" ]; then
        if [ "$(ls -A "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Test script cannot remove intermediate test results directory contents '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")' prior to test execution."
                complete_process 1
            fi
        fi
        if [ -d "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" ]; then
            if ! rmdir "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Test script cannot remove intermediate test results directory '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")' prior to test execution."
                complete_process 1
            fi
        fi
    fi

    if ! mkdir "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Test script cannot create test results directory '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")' to contain test results."
        complete_process 2
    fi
}

# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TEMP_FILE=/tmp/barebones.txt
TEST_RESULTS_DIRECTORY=test-results

# Parse any command line values.
parse_command_line "$@"

save_current_directory

# Create the test results directory forciably.
create_results_directory

# Various flags passed in.
echo "VERBOSE_MODE=$VERBOSE_MODE"
echo "NO_INIT_MODE=$NO_INIT_MODE"
echo "NUMBER_OF_THREADS=$NUMBER_OF_THREADS"
echo "TEST_DIRECTORY=$TEST_DIRECTORY"
echo "TEST_MODE=$TEST_MODE"

# File containing test properties. Only one property for now.
if [[ "$TEST_MODE" == "perf" ]] ; then
    echo "test-type=performance" > "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY/test.properties"
else
    echo "test-type=integration" > "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY/test.properties"
fi

# File containing the real data from the test.
echo "{ \"return-code\" : 0, \"sample-ms\" : 1 }" > "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY/test-summary.json"

# Add "configuration-file" if you want a summary of the rules configuration in the output.

# Workload properties specifies how to treat the data in the test-summary.json file
cp "$SCRIPTPATH/workload.properties" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"

# Can copy these into $TEST_RESULTS_DIRECTORY to provide extra insight:
# gaia_stats.log - will use to mine slices for information about rules
# gaia.log - currently will use to find exceptions

complete_process 0
