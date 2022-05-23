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
        echo "Testing the $PROJECT_NAME project against data in directory '$(realpath "$SCRIPTPATH/tests/$TEST_MODE")'."
    fi
}

# Simple function to stop the process, including any cleanup.
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 2 ] && [ "$SCRIPT_RETURN_CODE" -ne 4 ] ; then
        copy_test_output

        exec_summarize_test_results "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Testing of the $PROJECT_NAME project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Testing of the $PROJECT_NAME project succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    # Restore the current directory.
    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi

    popd > /dev/null 2>&1 || exit

    exit "$SCRIPT_RETURN_CODE"
}

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags] [test-name]"
    echo "Flags:"
    echo "  -l,--list                   List all available tests for this project."
    echo "  -d,--directory              Directory to run the tests against."
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

# Show a list of the available tests (directories).
list_available_tests() {
    echo "Available tests:"
    echo ""
    for next_file in ./tests/*
    do
        if [ -d "$next_file" ] ; then
            echo "${next_file:8}"
        fi
    done
}

# Parse the command line.
parse_command_line() {
    TEST_MODE="smoke"
    VERBOSE_MODE=0
    VERY_VERBOSE_MODE=0
    NO_INIT_MODE=0
    LIST_MODE=0
    NUMBER_OF_THREADS=-1
    PARAMS=()
    ALTERNATE_TEST_DIRECTORY=
    while (( "$#" )); do
    case "$1" in
        -vv|--very-verbose)
            VERBOSE_MODE=1
            VERY_VERBOSE_MODE=1
            shift
        ;;
        -ni|--no-init)
            NO_INIT_MODE=1
            shift
        ;;
        -d|--directory)
            # shellcheck disable=SC2086
            if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
                ALTERNATE_TEST_DIRECTORY=$2
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
        -l|--list)
            LIST_MODE=1
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

    if [ $LIST_MODE -ne 0 ] ; then
        list_available_tests
        complete_process 0
    fi

    if [ -n "$ALTERNATE_TEST_DIRECTORY" ] ; then
        TEST_DIRECTORY=$ALTERNATE_TEST_DIRECTORY
    fi

    if [[ ! "${PARAMS[0]}" == "" ]]; then
        TEST_MODE=${PARAMS[0]}
    fi

    verify_test_directory_exists
}

# Before getting to far, validate that the test directory exists
# and has the required files.
verify_test_directory_exists() {
    local DID_FAIL=0

    TEST_SOURCE_DIRECTORY=$SCRIPTPATH/tests/$TEST_MODE
    if [ ! -d "$TEST_SOURCE_DIRECTORY" ]; then
        echo "Test mode directory '$(realpath "$TEST_SOURCE_DIRECTORY")' does not exist."
        DID_FAIL=1
    elif [ ! -f "$TEST_SOURCE_DIRECTORY/commands.txt" ]; then
        echo "Test mode directory '$(realpath "$TEST_SOURCE_DIRECTORY")' does not contain a 'commands.txt' file."
        DID_FAIL=1
    fi
    if [ $DID_FAIL -ne 0 ]; then
        if [ "$(ls -A "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Intermediate test results directory '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")' could not be reset."
            fi
        fi
        complete_process 4
    fi
}

# Clear the test output directory, making sure it exists for the test execution.
clear_test_output() {
    if [ "$TEST_RESULTS_DIRECTORY" == "" ]; then
        complete_process 1 "Removing the specified directory '$TEST_RESULTS_DIRECTORY' is dangerous. Aborting."
    fi

    if [ -d "$TEST_RESULTS_DIRECTORY" ]; then
        if [ "$(ls -A "$TEST_RESULTS_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                complete_process 1 "Test script cannot remove intermediate test results directory '$(realpath "$TEST_RESULTS_DIRECTORY")' prior to test execution."
            fi
        fi
    else
        if ! mkdir "$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Test script cannot create intermediate test results directory '$(realpath "$TEST_RESULTS_DIRECTORY")' prior to test execution."
        fi
    fi

    MAIN_LOG=$LOG_DIRECTORY/gaia.log
    if [ -f "$MAIN_LOG" ]; then
        if ! rm "$MAIN_LOG" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Test script cannot remove general log '$MAIN_LOG' before running test."
        fi
    fi

    STATS_LOG=$LOG_DIRECTORY/gaia_stats.log
    if [ -f "$STATS_LOG" ]; then
        if ! rm "$STATS_LOG" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Test script cannot remove stats log '$STATS_LOG' before running test."
        fi
    fi
}

# Within the scope of completing this script, generate the test results.
exec_summarize_test_results() {
    local summarize_directory=$1

    if ! ./python/summarize_test_results.py "$summarize_directory" "$TEST_DIRECTORY/$GENERATED_CONFIGURATION_FILE" ; then
        complete_process 2 "Summarizing the results failed."
    fi
}

# Copy the intermediate test output directory to the script's test output directory.
copy_test_output() {
    if [ -d "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" ]; then
        if [ "$(ls -A "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")" ] ; then
            if ! rm "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Test script cannot remove test results directory '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")' to contain test results."
                complete_process 2
            fi
        fi
    else
        if ! mkdir "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot create test results directory '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")' to contain test results."
            complete_process 2
        fi
    fi

    if [ "$DID_PUSHD" -ne 0 ]; then
        if ! cp -r "$TEST_DIRECTORY/$GENERATED_CONFIGURATION_FILE" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1;  then
            cat "$TEMP_FILE"
            echo "Test script cannot copy generated configuration from '$(realpath "$TEST_DIRECTORY/$GENERATED_CONFIGURATION_FILE")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY/$GENERATED_CONFIGURATION_FILE")'."
            complete_process 2
        fi

        if ! cp -r "workload.properties" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1;  then
            cat "$TEMP_FILE"
            echo "Test script cannot copy workload properties from '$(realpath "workload.properties")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        if ! cp -r "tests/$TEST_MODE/test.properties" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1;  then
            cat "$TEMP_FILE"
            echo "Test script cannot copy test properties from '$(realpath "tests/$TEST_MODE/test.properties")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        if ! cp -r "$TEST_RESULTS_DIRECTORY/" "$SCRIPTPATH" > "$TEMP_FILE" 2>&1;  then
            cat "$TEMP_FILE"
            echo "Test script cannot copy intermediate test results from '$(realpath "$TEST_RESULTS_DIRECTORY")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        if ! cp "$LOG_DIRECTORY/gaia.log" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"  > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot copy intermediate gaia log files from '$(realpath "$LOG_DIRECTORY")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        if ! cp "$LOG_DIRECTORY/gaia_stats.log" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"  > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot copy intermediate stats log files from '$(realpath "$LOG_DIRECTORY")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        copy_extra_test_files

        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Test results located in: $(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")"
        fi
    fi
}

# Save the current directory when starting the script, so we can go back to that
# directory at the end of the script.
save_current_directory() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Saving current directory prior to test execution."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Test script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Change to the test directory for execution.
cd_to_test_directory() {
    if ! cd "$TEST_DIRECTORY"; then
        complete_process 1 "Test script cannot change to the test directory '$(realpath "$TEST_DIRECTORY")'."
    fi
}

# Remove the entire directory containing the test version of the project,
# reinstall the project therer, and rebuild the project there.
initialize_and_build_test_directory() {

    # To make sure we start at ground zero, remove the entire test directory.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Removing test directory '$(realpath "$TEST_DIRECTORY")' prior to test execution."
    fi
    if ! rm -rf "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        complete_process 1 "Test script cannot remove test directory '$(realpath "$TEST_DIRECTORY")' prior to test execution."
    fi

    # Install the project into the new test directory and cd into it.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing the project into test directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    if ! ./install.sh "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        complete_process 1 "Test script cannot install the project into directory '$(realpath "$TEST_DIRECTORY")'."
    fi

    cd_to_test_directory

    # Build the project.  Technically we don't need the -f flag, but it doesn't hurt either.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Building project in test directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    if [ "$VERY_VERBOSE_MODE" -ne 0 ]; then
        DID_FAIL=0
        ./build.sh -v -f
        DID_FAIL=$?
    else
        DID_FAIL=0
        ./build.sh -v -f > "$TEMP_FILE" 2>&1
        DID_FAIL=$?
    fi
    if [ "$DID_FAIL" -ne 0 ]; then
        if [ "$VERY_VERBOSE_MODE" -eq 0 ]; then
            cat "$TEMP_FILE"
        fi
        complete_process 1 "Test script cannot build the project in directory '$(realpath "$TEST_DIRECTORY")'."
    fi
}

# Execute the workflow that will run the specific test that was indicated.
execute_test_workflow() {

    TEST_SPECIFIC_PATH=$(realpath "$TEST_SOURCE_DIRECTORY/config.json")
    PROJECT_SPECIFIC_PATH=$(realpath "$TEST_SOURCE_DIRECTORY/../config.json")
    if [ -f "$TEST_SPECIFIC_PATH" ] ; then
        CONFIG_PATH=$TEST_SPECIFIC_PATH
        echo "Test specifc configuration specified.  Using '$TEST_SPECIFIC_PATH'."
    elif [ -f "$PROJECT_SPECIFIC_PATH" ] ; then
        CONFIG_PATH=$PROJECT_SPECIFIC_PATH
        echo "Project specifc configuration specified.  Using '$PROJECT_SPECIFIC_PATH'."
    else
        echo "No configuration specified.  Using default configuration."
    fi

    # Get rid of the reporting files in the build directory.
    rm "$BUILD_DIRECTORY/output.delay"
    rm "$BUILD_DIRECTORY/output.json"
    rm "$BUILD_DIRECTORY/output.csv"

    CONFIG_ARGUMENT=
    if [ -n "$CONFIG_PATH" ] ; then
        CONFIG_ARGUMENT="--config $CONFIG_PATH"
    fi

    THREAD_ARGUMENT=
    if [ "$NUMBER_OF_THREADS" -ge 0 ] ; then
        THREAD_ARGUMENT="-nt $NUMBER_OF_THREADS"
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Running debug commands through project in test directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    clear_test_output
    TEST_START_MARK=$(date +%s.%N)
    if [ "$VERY_VERBOSE_MODE" -ne 0 ]; then
        DID_FAIL=0
        # shellcheck disable=SC2086
        ./run.sh -v -c -a $THREAD_ARGUMENT $CONFIG_ARGUMENT -d "$TEST_DIRECTORY" "$TEST_COMMAND_NAME" "tests/$TEST_MODE/commands.txt"
        DID_FAIL=$?
    else
        DID_FAIL=0
        # shellcheck disable=SC2086
        ./run.sh -v -c -a $THREAD_ARGUMENT $CONFIG_ARGUMENT -d "$TEST_DIRECTORY" "$TEST_COMMAND_NAME" "tests/$TEST_MODE/commands.txt" > "$TEMP_FILE" 2>&1
        DID_FAIL=$?
    fi
    TEST_END_MARK=$(date +%s.%N)

    echo " { \"return-code\" : $DID_FAIL }" > "$TEST_RESULTS_DIRECTORY/return_code.json"

    # Make sure to calculate the runtime and store it in the `duration.json` file.
    # Note: the Python Json reader cannot parse a floating point value that does not
    #       have a `0` before it.  The `if` statement adds that required `0`.
    TEST_RUNTIME=$( echo "$TEST_END_MARK - $TEST_START_MARK" | bc -l )
    if [[ "$TEST_RUNTIME" == .* ]] ; then
        TEST_RUNTIME=0$TEST_RUNTIME
    fi

    echo "Test executed in $TEST_RUNTIME ms."
    echo " { \"duration\" : $TEST_RUNTIME }" > "$TEST_RESULTS_DIRECTORY/duration.json"

    if [ "$DID_FAIL" -ne 0 ]; then
        if [ "$VERY_VERBOSE_MODE" -eq 0 ]; then
            cat "$TEMP_FILE"
        fi
        echo "Test script cannot run the project in directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 5
    fi

    # If there is an expected output file, figure out if the expected results and
    # the actual output match.
    if [ -f "tests/$TEST_MODE/expected_output.json" ] && [ -f "$TEST_RESULTS_DIRECTORY/output.json" ]; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Verifying expected results against actual results."
        fi
        if ! diff -w "tests/$TEST_MODE/expected_output.json" "$TEST_RESULTS_DIRECTORY/output.json" > "$TEST_RESULTS_DIRECTORY/expected.diff" 2>&1 ; then
            echo "Test results were not as expected."
            echo "Differences between expected and actual results located at: $(realpath "$TEST_RESULTS_DIRECTORY/expected.diff")"
            complete_process 6
        fi
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "No expected results to verify against."
        fi
    fi
}

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091 source=./properties.sh
source "$SCRIPTPATH/properties.sh"

# Save the current directory before going on.
if ! pushd . > /dev/null 2>&1;  then
    complete_process 1 "Test script cannot save the current directory before proceeding."
    exit 1
fi

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.test.tmp

# Set up any local script variables.
DID_PUSHD=0

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process

# Save the current directory so we can get back here when done.
save_current_directory

# Unless specifically told not to initialize the tests, then do so.
if [ $NO_INIT_MODE -eq 0 ] ; then
    initialize_and_build_test_directory

# If we were not told to build the test directory, at least change to it.
else
    cd_to_test_directory
fi

# Run the sample test exection with the test mode that was selected.
# Capture the results and timing information on the run.
execute_test_workflow

# If we get here, we have a clean exit from the script.
complete_process 0
