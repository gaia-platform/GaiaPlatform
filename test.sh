#! /usr/bin/bash

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Testing the $PROJECT_NAME project against data in directory '$(realpath "$SCRIPTPATH/tests/$TEST_MODE")'."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1

    if [ "$SCRIPT_RETURN_CODE" -ne 2 ]; then
        copy_test_output
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

    exit "$SCRIPT_RETURN_CODE"
}

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags] [test-name]"
    echo "Flags:"
    echo "  -ni,--no-init       Do not initialize the test data before executing the test."
    echo "  -vv,--very-verbose  Verbose for this script and any top level scripts it calls."
    echo "  -v,--verbose        Show lots of information while executing the project."
    echo "  -h,--help           Display this help text."
    echo "Arguments:"
    echo "  test-name           Optional name of the test to run.  (Default: 'basic')"
    exit 1
}

# Parse the command line.
parse_command_line() {
    TEST_MODE="basic"
    VERBOSE_MODE=0
    VERY_VERBOSE_MODE=0
    NO_INIT_MODE=0
    PARAMS=()
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
    TEST_SOURCE_DIRECTORY=$SCRIPTPATH/tests/$TEST_MODE
    if [ ! -f "$TEST_SOURCE_DIRECTORY/commands.txt" ]; then
        echo "Test mode directory '$(realpath "$TEST_SOURCE_DIRECTORY")' does not contain a 'commands.txt' file."
        complete_process 1
    fi
    if [ ! -f "$TEST_SOURCE_DIRECTORY/expected_output.json" ]; then
        echo "Test mode directory '$(realpath "$TEST_SOURCE_DIRECTORY")' does not contain a 'expected_output.json' file."
        complete_process 1
    fi
}

# Clear the test output directory, making sure it exists for the test execution.
clear_test_output() {
    if [ "$TEST_RESULTS_DIRECTORY" == "" ]; then
        echo "Removing the specified directory '$TEST_RESULTS_DIRECTORY' is dangerous. Aborting."
        complete_process 1
    fi

    if [ -d "$TEST_RESULTS_DIRECTORY" ]; then
        if [ "$(ls -A "$TEST_RESULTS_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Test script cannot remove intermediate test results directory '$(realpath "$TEST_RESULTS_DIRECTORY")' prior to test execution."
                complete_process 1
            fi
        fi
    else
        if ! mkdir "$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot create intermediate test results directory '$(realpath "$TEST_RESULTS_DIRECTORY")' prior to test execution."
            complete_process 1
        fi
    fi

    STATS_LOG=$LOG_DIRECTORY/gaia_stats.log
    if [ -f "$STATS_LOG" ]; then
        if ! rm "$STATS_LOG" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot remove stats log '$STATS_LOG' before running test."
            complete_process 1
        fi
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
        if ! cp -r "$TEST_RESULTS_DIRECTORY/" "$SCRIPTPATH" > "$TEMP_FILE" 2>&1;  then
            cat "$TEMP_FILE"
            echo "Test script cannot copy intermediate test results from '$(realpath "$TEST_RESULTS_DIRECTORY")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        if ! cp "$BUILD_DIRECTORY"/output.* "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"  > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot copy intermediate test results from '$(realpath "$BUILD_DIRECTORY")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

        if ! cp "$LOG_DIRECTORY/gaia_stats.log" "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"  > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot copy intermediate log files from '$(realpath "$LOG_DIRECTORY")' to '$(realpath "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY")'."
            complete_process 2
        fi

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
        echo "Test script cannot save the current directory before proceeding."
        complete_process 1
    fi
    DID_PUSHD=1
}

# Change to the test directory for execution.
cd_to_test_directory() {
    if ! cd "$TEST_DIRECTORY"; then
        echo "Test script cannot change to the test directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 1
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
        echo "Test script cannot remove test directory '$(realpath "$TEST_DIRECTORY")' prior to test execution."
        complete_process 1
    fi

    # Install the project into the new test directory and cd into it.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing the project into test directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    if ! ./install.sh "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        echo "Test script cannot install the project into directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 1
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
        echo "Test script cannot build the project in directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 1
    fi
}

# Execute the workflow that will run the specific test that was indicated.
execute_test_workflow() {

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Running debug commands through project in test directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    clear_test_output
    TEST_START_MARK=$(date +%s.%N)
    if [ "$VERY_VERBOSE_MODE" -ne 0 ]; then
        DID_FAIL=0
        ./run.sh -v -a "$TEST_COMMAND_NAME" "tests/$TEST_MODE/commands.txt"
        DID_FAIL=$?
    else
        DID_FAIL=0
        ./run.sh -v -a "$TEST_COMMAND_NAME" "tests/$TEST_MODE/commands.txt" > "$TEMP_FILE" 2>&1
        DID_FAIL=$?
    fi
    TEST_END_MARK=$(date +%s.%N)
    if [ "$DID_FAIL" -ne 0 ]; then
        if [ "$VERY_VERBOSE_MODE" -eq 0 ]; then
            cat "$TEMP_FILE"
        fi
        echo "Test script cannot run the project in directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 1
    fi

    # Make sure to calculate the runtime and store it in the `duration.json` file.
    TEST_RUNTIME=$( echo "$TEST_END_MARK - $TEST_START_MARK" | bc -l )
    echo "Test executed in $TEST_RUNTIME ms."
    echo " { \"duration\" : $TEST_RUNTIME }" > "$TEST_RESULTS_DIRECTORY/duration.json"

    # Figure out if the expected results and the actual output match.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Verifying expected results against actual results."
    fi
    if ! diff "tests/$TEST_MODE/expected_output.json" "$BUILD_DIRECTORY/output.json" > "$TEST_RESULTS_DIRECTORY/expected.diff" 2>&1 ; then
        echo "Test results were not as expected."
        echo "Differences between expected and actual results located at: $(realpath "$TEST_RESULTS_DIRECTORY/expected.diff")"
        complete_process 1
    fi
}



# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091
source "$SCRIPTPATH/properties.sh"

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
