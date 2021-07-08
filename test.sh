#! /usr/bin/bash

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Testing the incubator..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    # $1 is the return code to assign to the script

    copy_test_output

    if [ "$1" -ne 0 ]; then
        echo "Testing of the incubator failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Testing of the incubator succeeded."
        fi
    fi

    # Restore the current directory.
    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi
    exit "$1"
}

# Show how this script can be used.
show_usage() {
    echo "Usage: $(basename "$0") [flags]"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while executing the project."
    echo "  -h,--help         Display this help text."
    echo "  -ni,--no-init     Do not initialize the test data before executing the test."
    exit 1
}

# Clear the test output directory, making sure it exists for the test execution.
clear_test_output() {
    if [ -d "$TEST_RESULTS_DIRECTORY" ]; then
        if ! rm "$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot remove intermediate test results directory '$TEST_RESULTS_DIRECTORY' prior to test execution."
            complete_process 1
        fi
    else
        if ! mkdir "$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot create intermediate test results directory '$TEST_RESULTS_DIRECTORY' prior to test execution."
            complete_process 1
        fi
    fi
}

# Copy the intermediate test output directory to the script's test output directory.
copy_test_output() {
    if [ -d "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" ]; then
        if ! rm "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot remove test results directory '$SCRIPTPATH/$TEST_RESULTS_DIRECTORY' to contain test results."
            complete_process 1
        fi
    else
        if ! mkdir "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Test script cannot create test results directory '$SCRIPTPATH/$TEST_RESULTS_DIRECTORY' to contain test results."
            complete_process 1
        fi
    fi

    if ! cp -r "$TEST_RESULTS_DIRECTORY/" "$SCRIPTPATH" > "$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        echo "Test script cannot intermediate test results from '$TEST_RESULTS_DIRECTORY' to '$SCRIPTPATH/$TEST_RESULTS_DIRECTORY'."
        complete_process 1
    fi

    if ! cp build/output.* "$SCRIPTPATH/$TEST_RESULTS_DIRECTORY"  > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Test script cannot intermediate test results from 'build' to '$SCRIPTPATH/$TEST_RESULTS_DIRECTORY'."
        complete_process 1
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
        echo "Test script cannot change to the test directory '$TEST_DIRECTORY'."
        complete_process 1
    fi
}

# Remove the entire directory containing the test version of the project,
# reinstall the project therer, and rebuild the project there.
initialize_and_build_test_directory() {
    # To make sure we start at ground zero, remove the entire test directory.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Removing test directory '$TEST_DIRECTORY' prior to test execution."
    fi
    if ! rm -rf "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        echo "Test script cannot remove test directory '$TEST_DIRECTORY' prior to test execution."
        complete_process 1
    fi

    # Install the project into the new test directory and cd into it.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing project into test directory '$TEST_DIRECTORY'."
    fi
    if ! ./install.sh "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        echo "Test script cannot install the project into directory '$TEST_DIRECTORY'."
        complete_process 1
    fi

    cd_to_test_directory

    # Build the project.  Technically we don't need the -f flag, but it doesn't hurt either.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Building project in test directory '$TEST_DIRECTORY'."
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
        echo "Test script cannot build the project in directory '$TEST_DIRECTORY'."
        complete_process 1
    fi
}

# Execute the workflow that will run the specific test that was indicated.
execute_test_workflow() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Running debug commands through project in test directory '$TEST_DIRECTORY'."
    fi
    clear_test_output
    TEST_START_MARK=$(date +%s.%N)
    if [ "$VERY_VERBOSE_MODE" -ne 0 ]; then
        DID_FAIL=0
        ./run.sh -v -a debug "tests/$TEST_MODE/commands.txt"
        DID_FAIL=$?
    else
        DID_FAIL=0
        ./run.sh -v -a debug "tests/$TEST_MODE/commands.txt" > "$TEMP_FILE" 2>&1
        DID_FAIL=$?
    fi
    TEST_END_MARK=$(date +%s.%N)
    if [ "$DID_FAIL" -ne 0 ]; then
        if [ "$VERY_VERBOSE_MODE" -eq 0 ]; then
            cat "$TEMP_FILE"
        fi
        echo "Test script cannot run the project in directory '$TEST_DIRECTORY'."
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
    if ! diff "tests/$TEST_MODE/expected_output.json" "build/output.json" > "$TEST_RESULTS_DIRECTORY/expected.diff" 2>&1 ; then
        echo "Test results were not as expected."
        echo "Differences between expected and actual results located at: $TEST_RESULTS_DIRECTORY/expected.diff"
        complete_process 1
    fi
}

# Set up any script variables.
DID_PUSHD=0
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TEST_DIRECTORY=/tmp/test_incubator
TEST_RESULTS_DIRECTORY="test-results"
TEMP_FILE=/tmp/incubator.test.tmp

# Parse any command line values.
TEST_MODE="basic"
VERBOSE_MODE=0
VERY_VERBOSE_MODE=0
NO_INIT_MODE=0
PARAMS=""
while (( "$#" )); do
  case "$1" in
    -h|--help) # unsupported flags
      show_usage
      ;;
    -v|--verbose)
      VERBOSE_MODE=1
      shift
      ;;
    -vv|--very-verbose)
      VERBOSE_MODE=1
      VERY_VERBOSE_MODE=1
      shift
      ;;
    -ni|--no-init)
      NO_INIT_MODE=1
      shift
      ;;
    -*) # unsupported flags
      echo "Error: Unsupported flag $1" >&2
      show_usage
      ;;
    *) # preserve positional arguments
      PARAMS="$PARAMS $1"
      shift
      ;;
  esac
done
eval set -- "$PARAMS"

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
complete_process 0 "$1"
