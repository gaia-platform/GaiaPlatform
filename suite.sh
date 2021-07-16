#! /usr/bin/bash

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Testing the $PROJECT_NAME project against suite ..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Testing of the $PROJECT_NAME suite failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Testing of the $PROJECT_NAME suite succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    # Restore the current directory.
    if [ "$DID_PUSHD_FOR_BUILD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi
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
    echo "  -l,--list           List all available suites for this project."
    echo "  -v,--verbose        Show lots of information while executing the suite of tests."
    echo "  -h,--help           Display this help text."
    echo "Arguments:"
    echo "  suite-name          Optional name of the suite to run.  (Default: 'smoke')"
    exit 1
}

# Show a list of the available suites (suite files).
list_available_suites() {
    echo "Available suites:"
    echo ""
    for next_file in ./tests/*
    do
        if [ -f "$next_file" ] ; then
            next_file_end="${next_file:8}"
            if [[ $next_file_end = suite-* ]] ; then
                echo "$next_file_end"
            fi
        fi
    done
}

# Parse the command line.
parse_command_line() {
    SUITE_MODE="smoke"
    VERBOSE_MODE=0
    LIST_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
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
        list_available_suites
        complete_process 0
    fi

    if [[ ! "${PARAMS[0]}" == "" ]]; then
        SUITE_MODE=${PARAMS[0]}
    fi
    TEST_SOURCE_DIRECTORY=$SCRIPTPATH/tests
    SUITE_FILE_NAME=$TEST_SOURCE_DIRECTORY/suite-${SUITE_MODE}.txt
    if [ ! -f "$SUITE_FILE_NAME" ]; then
        echo "Test directory '$(realpath "$TEST_SOURCE_DIRECTORY")' does not contain a 'suite-${SUITE_MODE}.txt' file."
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
        echo "Suite script cannot save the current directory before proceeding."
        complete_process 1
    fi
    DID_PUSHD=1
}

# Clear the suite output directory, making sure it exists for the suite execution.
clear_suite_output() {
    if [ "$SUITE_RESULTS_DIRECTORY" == "" ]; then
        echo "Removing the specified directory '$SUITE_RESULTS_DIRECTORY' is dangerous. Aborting."
        complete_process 1
    fi

    if [ -d "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY" ]; then
        if [ "$(ls -A "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Suite script cannot remove suite results directory '$(realpath "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY")' prior to suite execution."
                complete_process 1
            fi
        fi
    else
        if ! mkdir "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            echo "Suite script cannot create suite results directory '$(realpath "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY")' prior to suite execution."
            complete_process 1
        fi
    fi
}

# Ensure that we have a current and fully built project in our "test" directory
# before proceeding with the tests.
install_and_build_cleanly() {

    # Remove any existing directory.
    if ! rm -rf "$TEST_DIRECTORY"; then
        echo "Suite script failed to remove the '$(realpath "$TEST_DIRECTORY")' directory."
        complete_process 1
    fi

    # Install a clean version into that directory.
    echo "Suite script installing the project into directory '$(realpath "$TEST_DIRECTORY")'..."
    if ! ./install.sh "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        echo "Suite script failed to install the project into directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 1
    fi
    echo "Suite script installed the project into directory '$(realpath "$TEST_DIRECTORY")'."
    if ! pushd "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Suite script cannot push directory to '$(realpath "$TEST_DIRECTORY")'."
    fi
    DID_PUSHD_FOR_BUILD=1

    # Build the executable, tracking any build information for later examination.
    echo "Suite script building the project in directory '$(realpath "$TEST_DIRECTORY")'..."
    if ! ./build.sh -v  > "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/build.txt" 2>&1 ; then
        cat "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/build.txt"
        echo "Suite script failed to build the project in directory '$(realpath "$TEST_DIRECTORY")'."
        complete_process 1
    fi
    echo "Suite script built the project in directory '$(realpath "$TEST_DIRECTORY")'."

    if ! popd > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Suite script cannot pop directory to restore state."
    fi
    DID_PUSHD_FOR_BUILD=0
}

# Execute a single test within the test suite.
execute_suite_test() {
    local NEXT_TEST_NAME=$1

    # Let the runner know what is going on.
    echo "Executing test: $NEXT_TEST_NAME"

    # Make a new directory to contain all the results of the test.
    SUITE_TEST_DIRECTORY=$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/$NEXT_TEST_NAME
    if ! mkdir "$SUITE_TEST_DIRECTORY" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Suite script cannot create suite test directory '$(realpath "$SUITE_TEST_DIRECTORY")' prior to test execution."
        complete_process 1
    fi

    # Execute the test in question.  Note that we do not complete the script
    # if there are any errors, we just make sure that information is noted in
    # the return_code.json file.
    ./test.sh -vv -ni "$NEXT_TEST_NAME" > "$SUITE_TEST_DIRECTORY/output.txt" 2>&1
    TEST_RETURN_CODE=$?
    echo " { \"return-code\" : $TEST_RETURN_CODE }" > "$TEST_RESULTS_DIRECTORY/return_code.json"

    # Copy any files in the `test-results` directory into a test-specific directory for
    # that one test.
    if ! cp -r "$TEST_RESULTS_DIRECTORY"/* "$SUITE_TEST_DIRECTORY" > "$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        echo "Suite script cannot copy test results from '$(realpath "$TEST_RESULTS_DIRECTORY")' to '$(realpath "$SUITE_TEST_DIRECTORY")'."
        complete_process 2
    fi
}



# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091
source "$SCRIPTPATH/properties.sh"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.suite.tmp

# Set up any local script variables.
DID_PUSHD=0
DID_PUSHD_FOR_BUILD=0

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process

save_current_directory

clear_suite_output

install_and_build_cleanly

IFS=$'\r\n' GLOBIGNORE='*' command eval  'TEST_NAMES=($(cat $SUITE_FILE_NAME))'
for NEXT_TEST_NAME in "${TEST_NAMES[@]}"; do
    execute_suite_test "$NEXT_TEST_NAME"
done

if ! ./python/summarize_results.py "$SUITE_FILE_NAME"; then
    echo "Summarizing the results failed."
    complete_process 1
fi

# If we get here, we have a clean exit from the script.
complete_process 0
