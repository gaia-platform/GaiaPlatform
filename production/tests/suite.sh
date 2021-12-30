#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Testing the $SUITE_MODE suite ..."
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
        COMPLETE_MESSAGE="Testing the $SUITE_MODE suite failed."
        echo "$COMPLETE_MESSAGE"
    else
        COMPLETE_MESSAGE="Testing the $SUITE_MODE suite succeeded."
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "$COMPLETE_MESSAGE"
        fi
    fi

    kill_database_if_was_started

    if [ -n "$WORKLOAD_DIRECTORY" ] ; then
        if [ -f "$WORKLOAD_DIRECTORY/template.sh" ]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Removing workload template."
            fi
            if ! "$WORKLOAD_DIRECTORY"/template.sh -v -r > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                echo "Workload template cannot be removed. Please remove manually."
            fi
        fi
    fi


    if [ "$DID_REPORT_START" -ne 0 ] ; then
        if [ "$SLACK_MODE" -ne 0 ] ; then
            "$SCRIPTPATH/python/publish_to_slack.py" message "$SUITE_MODE" "$COMPLETE_MESSAGE"
            if [ -n "$COMPLETE_REASON" ] ; then
                ./python/publish_to_slack.py message "$SUITE_MODE" "Failure reason: $COMPLETE_REASON"
            fi
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
    echo "  -n,--no-stats       Do not display the statistics when the suite has completed."
    echo "  -j,--json           Display the statistics in JSON format."
    echo "  -d,--database       Start a new instance of the database for the tests within the suite."
    echo "  -p,--persistence    Enable persistence when starting a new instance of the database."
    echo "  -m,--memory         Track memory usage for associated applications."
    echo "  -f,--force-db-stop  Force any existing database to be stopped prior to starting a new database."
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
    echo "Arguments:"
    echo "  suite-name          Optional name of the suite to run.  (Default: 'smoke')"
    exit 1
}

# Take the specified message and broadcast it to the command line and, if
# enabled, slack.
broadcast_message() {
    local title=$1
    local message=$2

    echo "$2"
    if [ "$SLACK_MODE" -ne 0 ] ; then
        "$SCRIPTPATH/python/publish_to_slack.py" message "$title" "$message"
    fi
}

# Show a list of the available suites (suite files).
list_available_suites() {
    echo "Available suites:"
    echo ""
    for next_file in ./suite-definitions/*
    do
        if [ -f "$next_file" ] ; then
            next_file_end="${next_file:8}"
            if [[ $next_file_end = suite-* ]] && [[ $next_file_end = *.txt ]]; then
                echo "${next_file_end:6:-4}"
            fi
        fi
    done
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    SLACK_MODE=0
    LIST_MODE=0
    SHOW_STATS=1
    SHOW_JSON_STATS=0
    START_DATABASE=0
    TRACK_MEMORY=0
    FORCE_DATABASE_RESTART=0
    START_DATABASE_WITH_PERSISTENCE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -l|--list)
            LIST_MODE=1
            shift
        ;;
        -s|--slack)
            SLACK_MODE=1
            shift
        ;;
        -j|--json)
            SHOW_JSON_STATS=1
            shift
        ;;
        -n|--no-stats)
            SHOW_STATS=0
            shift
        ;;
        -m|--memory)
            TRACK_MEMORY=1
            shift
        ;;
        -d|--database)
            START_DATABASE=1
            shift
        ;;
        -p|--persistence)
            START_DATABASE=1
            START_DATABASE_WITH_PERSISTENCE=1
            shift
        ;;
        -f|--force-db-stop)
            FORCE_DATABASE_RESTART=1
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
    SUITE_SOURCE_DIRECTORY=$SCRIPTPATH/suite-definitions
    SUITE_FILE_NAME=$SUITE_SOURCE_DIRECTORY/suite-${SUITE_MODE}.txt
    if [ ! -f "$SUITE_FILE_NAME" ]; then
        complete_process 1 "Test directory '$(realpath "$SUITE_SOURCE_DIRECTORY")' does not contain a 'suite-${SUITE_MODE}.txt' file."
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
        complete_process 1 "Suite script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Clear the suite output directory, making sure it exists for the suite execution.
clear_suite_output() {
    if [ "$SUITE_RESULTS_DIRECTORY" == "" ]; then
        complete_process 1 "Removing the specified directory '$SUITE_RESULTS_DIRECTORY' is dangerous. Aborting."
    fi

    if [ -d "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY" ]; then
        if [ "$(ls -A "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY")" ] ; then
            # shellcheck disable=SC2115
            if ! rm -rf "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY"/* > "$TEMP_FILE" 2>&1; then
                cat "$TEMP_FILE"
                complete_process 1 "Suite script cannot remove suite results directory '$(realpath "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY")' prior to suite execution."
            fi
        fi
    else
        if ! mkdir "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY" > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Suite script cannot create suite results directory '$(realpath "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY")' prior to suite execution."
        fi
    fi
}

kill_database_if_was_started() {

    if [ -n "$DB_SERVER_PID" ] ; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Killing started database server."
        fi
        kill "$DB_SERVER_PID"
        DB_SERVER_PID=
    fi
}

start_database_if_required() {
    if [ "$START_DATABASE" -ne 0 ]; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Creating a new database service instance for the test suite."
        fi
        SERVER_PID=$(pgrep "gaia_db_server")
        if [[ -n $SERVER_PID ]] ; then
            if [ $FORCE_DATABASE_RESTART -eq 0 ] ; then
                complete_process 1 "Test suite specified a new database service instance, but one is already running. Stop that instance before trying again."
            fi

            if ! kill "$SERVER_PID" ; then
                complete_process 1 "Test suite failed to stop existing database service instance. Stop that instance manually before trying again."
            fi
            sleep 2
            SERVER_PID=$(pgrep "gaia_db_server")
            if [[ -n $SERVER_PID ]] ; then
                complete_process 1 "Test suite verification of stopped database service instance failed. Stop that instance manually before trying again."
            fi
        fi

        if [ -d "$SUITE_RESULTS_DIRECTORY/dbdir" ] ; then
            if ! rm -rf "$SUITE_RESULTS_DIRECTORY/dbdir" > "$TEMP_FILE" 2>&1 ; then
                cat "$TEMP_FILE"
                complete_process 1 "Cannot remove the old database directory before starting the new database service instance."
            fi
        fi

        PERSISTENCE_FLAG="--persistence disabled"
        if [ $START_DATABASE_WITH_PERSISTENCE -ne 0 ] ; then
            if ! mkdir "$SUITE_RESULTS_DIRECTORY/dbdir" > "$TEMP_FILE" 2>&1 ; then
                cat "$TEMP_FILE"
                complete_process 1 "Cannot create the requested database directory."
            fi

            PERSISTENCE_FLAG="--persistence enabled --data-dir $SUITE_RESULTS_DIRECTORY/dbdir"
        fi

        # shellcheck disable=SC2086
        gaia_db_server $PERSISTENCE_FLAG > "$SUITE_RESULTS_DIRECTORY/database.log" 2>&1 &
        # shellcheck disable=SC2181
        if [ $? -ne 0 ] ; then
            cat "$SUITE_RESULTS_DIRECTORY/database.log"
            complete_process 1 "Cannot start the requested database service instance."
        fi

        # Five seconds is way more than enough for 95% of the cases, but may be needed in
        # some cases with persistence.
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Pausing for 5 seconds before verifying that the new database service instance is responsive."
        fi
        DB_SERVER_PID=$(pgrep "gaia_db_server")
        sleep 5
        DB_SERVER_PID=
        SERVER_PID=$(pgrep "gaia_db_server")
        if [[ -z $SERVER_PID ]] ; then
            cat "$SUITE_RESULTS_DIRECTORY/database.log"
            complete_process 1 "Newly started database service instance was not responsive.  Cannot continue the test suite without an active database instance."
        fi
        DB_SERVER_PID=$SERVER_PID
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Creating a new database service instance for the test suite."
        fi

        # Make sure that we register a trap handler to be able to kill the database
        # if the suite stops.
        trap kill_database_if_was_started EXIT
    fi
}

# Based on the suite file, determine the workload directory.
determine_workload_directory() {

    WORKLOAD_NAME=
    WORKLOAD_DIRECTORY=
    # shellcheck disable=SC2016
    IFS=$'\r\n' GLOBIGNORE='*' command eval  'TEST_NAMES=($(cat $SUITE_FILE_NAME))'
    for NEXT_TEST_NAME in "${TEST_NAMES[@]}"; do
        SUB="^[[:space:]]*#"
        if [[ "$NEXT_TEST_NAME" =~ $SUB ]] || [[ -z "${NEXT_TEST_NAME// }" ]]; then
            continue
        fi
        SUB="^workload[[:space:]]+(.*)$"
        if [[ "$NEXT_TEST_NAME" =~ $SUB ]]; then
            if [ -n "$WORKLOAD_DIRECTORY" ] ; then
                complete_process 1 "Suite file '$SUITE_FILE_NAME' cannot specify multiple workloads."
            fi
            WORKLOAD_DIRECTORY="${BASH_REMATCH[1]}"
        fi
    done

    if [[ -z $WORKLOAD_DIRECTORY ]] ; then
        complete_process 1 "Suite file '$SUITE_FILE_NAME' must specify a workload."
    fi

    # shellcheck disable=SC1001
    if [[ "$WORKLOAD_DIRECTORY" != *\/* ]] ; then
        WORKLOAD_NAME="$WORKLOAD_DIRECTORY"
        WORKLOAD_DIRECTORY=$SCRIPTPATH/workloads/$WORKLOAD_DIRECTORY
    fi
}

# Verify the contents of the workload directory.
# For more, go to XXX.
verify_workload_directory() {

    if [ ! -d "$WORKLOAD_DIRECTORY" ]; then
        complete_process 1 "Workload directory '$WORKLOAD_DIRECTORY' must exist."
    fi

    if [ -f "$WORKLOAD_DIRECTORY/template.sh" ]; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Applying workload template."
        fi
        if ! "$WORKLOAD_DIRECTORY"/template.sh -v > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Workload template cannot be appplied."
        fi
    fi

    if [ ! -f "$WORKLOAD_DIRECTORY/install.sh" ]; then
        complete_process 1 "Workload directory '$WORKLOAD_DIRECTORY' must specify an 'install.sh' script."
    fi
    if [[ ! -x "$WORKLOAD_DIRECTORY/install.sh" ]] ; then
        complete_process 1 "Workload file '$WORKLOAD_DIRECTORY/install.sh' must be executable."
    fi

    if [ ! -f "$WORKLOAD_DIRECTORY/build.sh" ]; then
        complete_process 1 "Workload directory '$WORKLOAD_DIRECTORY' must specify a 'build.sh' script."
    fi
    if [[ ! -x "$WORKLOAD_DIRECTORY/build.sh" ]] ; then
        complete_process 1 "Workload file '$WORKLOAD_DIRECTORY/build.sh' must be executable."
    fi

    if [ ! -f "$WORKLOAD_DIRECTORY/test.sh" ]; then
        complete_process 1 "Workload directory '$WORKLOAD_DIRECTORY' must specify a 'test.sh' script."
    fi
    if [[ ! -x "$WORKLOAD_DIRECTORY/test.sh" ]] ; then
        complete_process 1 "Workload file '$WORKLOAD_DIRECTORY/test.sh' must be executable."
    fi
}

# Ensure that we have a current and fully built project in our "test" directory
# before proceeding with the tests.
install_and_build_cleanly() {

    broadcast_message "$SUITE_MODE" "Installing test suite project in temporary directory."

    # Remove any existing directory.
    # shellcheck disable=SC2153
    if ! rm -rf "$TEST_DIRECTORY"; then
        complete_process 1 "Suite script failed to remove the '$(realpath "$TEST_DIRECTORY")' directory."
    fi

    # Install a clean version into that directory.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Suite script installing the project into directory '$(realpath "$TEST_DIRECTORY")'..."
    fi
    if ! "$WORKLOAD_DIRECTORY/install.sh" "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        complete_process 1 "Suite script failed to install the project into directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Suite script installed the project into directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    if ! pushd "$TEST_DIRECTORY" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Suite script cannot push directory to '$(realpath "$TEST_DIRECTORY")'."
    fi
    DID_PUSHD_FOR_BUILD=1

    broadcast_message "$SUITE_MODE" "Building test suite project in temporary directory."

    # Build the executable, tracking any build information for later examination.
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Suite script building the project in directory '$(realpath "$TEST_DIRECTORY")'..."
    fi
    if ! ./build.sh --verbose  > "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/build.txt" 2>&1 ; then
        cat "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/build.txt"
        complete_process 1 "Suite script failed to build the project in directory '$(realpath "$TEST_DIRECTORY")'."
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Suite script built the project in directory '$(realpath "$TEST_DIRECTORY")'."
    fi

    if ! popd > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        echo "Suite script cannot pop directory to restore state."
    fi
    DID_PUSHD_FOR_BUILD=0
}

# Execute a single test within the test suite.
execute_single_test() {
    local NEXT_TEST_NAME=$1
    local REPEAT_NUMBER=$2

    broadcast_message "" "Sleeping for $PAUSE_IN_SECONDS_BEFORE_NEXT_TEST seconds before next test."
    sleep "$PAUSE_IN_SECONDS_BEFORE_NEXT_TEST"

    # Let the runner know what is going on.
    if [ -n "$REPEAT_NUMBER" ] ; then
        broadcast_message "" "Executing test: $NEXT_TEST_NAME, repeat $REPEAT_NUMBER"
    else
        broadcast_message "" "Executing test: $NEXT_TEST_NAME"
    fi

    # Make a new directory to contain all the results of the test.
    SUITE_TEST_DIRECTORY=$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/$NEXT_TEST_NAME
    if [ -n "$REPEAT_NUMBER" ] ; then
        SUITE_TEST_DIRECTORY="$SUITE_TEST_DIRECTORY"_"$REPEAT_NUMBER"
    fi
    if [ -d "$SUITE_TEST_DIRECTORY" ] ; then
        MAX_ATTEMPTS=99
        FINAL_DIRECTORY=
        for (( DIRECTORY_SUFFIX_INDEX=1; DIRECTORY_SUFFIX_INDEX<=MAX_ATTEMPTS; DIRECTORY_SUFFIX_INDEX++ ))
          do
            NEXT_DIRECTORY="${SUITE_TEST_DIRECTORY}__$DIRECTORY_SUFFIX_INDEX"
            if [ ! -d "$NEXT_DIRECTORY" ] ; then
                FINAL_DIRECTORY=$NEXT_DIRECTORY
                break
            fi
          done

        if [ "$MAX_ATTEMPTS" -eq "$DIRECTORY_SUFFIX_INDEX" ] ; then
            complete_process 1 "Failed to find a results directory for '$SUITE_TEST_DIRECTORY' test."
        fi
        SUITE_TEST_DIRECTORY=$FINAL_DIRECTORY
    fi

    TEST_THREADS_ARGUMENT=
    if [ -n "$NUMBER_OF_THREADS" ] ; then
        TEST_THREADS_ARGUMENT="--num-threads $NUMBER_OF_THREADS"
    fi

    # Make sure to record the eventual directory that we used so we can summarize it more effectively.
    echo "$SUITE_TEST_DIRECTORY" >> "$EXECUTE_MAP_FILE"

    if ! mkdir "$SUITE_TEST_DIRECTORY" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Suite script cannot create suite test directory '$(realpath "$SUITE_TEST_DIRECTORY")' prior to test execution."
    fi

    # Execute the test in question.  Note that we do not complete the script
    # if there are any errors, we just make sure that information is noted in
    # the return_code.json file.

    # shellcheck disable=SC2086
    $WORKLOAD_DIRECTORY/test.sh --very-verbose --no-init $TEST_THREADS_ARGUMENT --directory "$TEST_DIRECTORY" "$NEXT_TEST_NAME" > "$SUITE_TEST_DIRECTORY/output.txt" 2>&1
    TEST_RETURN_CODE=$?

    # Copy any files in the `test-results` directory into a test-specific directory for
    # that one test.
    test_files=$(shopt -s nullglob dotglob; echo "$WORKLOAD_DIRECTORY/$TEST_RESULTS_DIRECTORY"/*)
    if (( ${#test_files} )) ; then
        if ! cp -r "$WORKLOAD_DIRECTORY/$TEST_RESULTS_DIRECTORY"/* "$SUITE_TEST_DIRECTORY" > "$TEMP_FILE" 2>&1;  then
            cat "$TEMP_FILE"
            complete_process 2 "Suite script cannot copy test results from '$(realpath "$TEST_RESULTS_DIRECTORY")' to '$(realpath "$SUITE_TEST_DIRECTORY")'."
        fi
    fi
    echo " { \"return-code\" : $TEST_RETURN_CODE }" > "$SUITE_TEST_DIRECTORY/return_code2.json"
}

# Execute a test within the test suite.
execute_suite_test() {
    local NEXT_TEST_NAME=$1

    SUB="^[[:space:]]*#"
    if [[ "$NEXT_TEST_NAME" =~ $SUB ]] || [[ -z "${NEXT_TEST_NAME// }" ]]; then
        return
    fi

    SUB="^workload[[:space:]]+(.*)$"
    if [[ "$NEXT_TEST_NAME" =~ $SUB ]] ; then
        return
    fi

    NUMBER_OF_THREADS=
    SUB="^(.*)[[:space:]]+threads[[:space:]]+([0-9]+)(.*)$"
    if [[ "$NEXT_TEST_NAME" =~ $SUB ]]; then
        NEXT_TEST_NAME="${BASH_REMATCH[1]}"
        NUMBER_OF_THREADS="${BASH_REMATCH[2]}"
        REMAINING_TEXT="${BASH_REMATCH[3]}"
        NEXT_TEST_NAME="$NEXT_TEST_NAME$REMAINING_TEXT"
    fi

    SUB="^(.*)[[:space:]]+repeat[[:space:]]+([0-9]+)$"
    if [[ "$NEXT_TEST_NAME" =~ $SUB ]]; then
        NEXT_TEST_NAME="${BASH_REMATCH[1]}"
        NUMBER_OF_REPEATS="${BASH_REMATCH[2]}"
        if [ "$NUMBER_OF_REPEATS" -le 0 ]; then
            complete_process 2 "Repeat value $NUMBER_OF_REPEATS for test name '$NEXT_TEST_NAME' is not a positive integer."
        fi
        broadcast_message "" "Executing suite test: $NEXT_TEST_NAME with $NUMBER_OF_REPEATS repeats."

        # Make sure to record the suite so we can summarize it more effectively.
        echo "Suite $NEXT_TEST_NAME" >> "$EXECUTE_MAP_FILE"
        for (( TEST_NUMBER=1; TEST_NUMBER<=NUMBER_OF_REPEATS; TEST_NUMBER++ ))
          do
            if [ $TRACK_MEMORY -ne 0 ] ; then
                CURRENT_TIMESTAMP=$(python3 -c 'import time; print(int(time.time()))')
                echo "$CURRENT_TIMESTAMP - Starting suite test: $NEXT_TEST_NAME" >> "$MEMORY_INDEX_FILE"
            fi
            execute_single_test "$NEXT_TEST_NAME" "$TEST_NUMBER"
            if [ $TRACK_MEMORY -ne 0 ] ; then
                CURRENT_TIMESTAMP=$(python3 -c 'import time; print(int(time.time()))')
                echo "$CURRENT_TIMESTAMP - Completed suite test: $NEXT_TEST_NAME" >> "$MEMORY_INDEX_FILE"
                sleep 1
            fi
          done
        echo "EndSuite $NEXT_TEST_NAME" >> "$EXECUTE_MAP_FILE"
    else
        if [ $TRACK_MEMORY -ne 0 ] ; then
            CURRENT_TIMESTAMP=$(python3 -c 'import time; print(int(time.time()))')
            echo "$CURRENT_TIMESTAMP - Starting suite test: $NEXT_TEST_NAME" >> "$MEMORY_INDEX_FILE"
        fi
        execute_single_test "$NEXT_TEST_NAME"
        if [ $TRACK_MEMORY -ne 0 ] ; then
            CURRENT_TIMESTAMP=$(python3 -c 'import time; print(int(time.time()))')
            echo "$CURRENT_TIMESTAMP - Completed suite test: $NEXT_TEST_NAME" >> "$MEMORY_INDEX_FILE"
            sleep 1
        fi
    fi
}



# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Relative directory where the test suite results are stored in this project.
SUITE_RESULTS_DIRECTORY="suite-results"

# Relative directory where the test results are stored after being executed in the workload.
TEST_RESULTS_DIRECTORY="test-results"

# Set up any project based local script variables.
SUITE_MODE="smoke"
TEMP_FILE=/tmp/$SUITE_MODE.suite.tmp
EXECUTE_MAP_FILE=$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/map.txt
TEST_DIRECTORY=/tmp/test_suite

# Set up any local script variables.
PAUSE_IN_SECONDS_BEFORE_NEXT_TEST=15

DID_PUSHD=0
DID_PUSHD_FOR_BUILD=0
DID_REPORT_START=0
WORKLOAD_DIRECTORY=
DB_SERVER_PID=

# Parse any command line values.
parse_command_line "$@"

if [ $START_DATABASE -eq 0 ] ; then
    if [ $FORCE_DATABASE_RESTART -ne 0 ] ; then
        complete_process 1 "Cannot force a database reset without requesting to start a new database instance."
    fi
    if [ $START_DATABASE_WITH_PERSISTENCE -ne 0 ] ; then
        complete_process 1 "Cannot activate database persistence without requesting to start a new database instance."
    fi
fi

determine_workload_directory
verify_workload_directory

# Reset this after the command line, now that we know what the selected
# SUITE_MODE is.
TEMP_FILE=/tmp/$SUITE_MODE.suite.tmp
MEMORY_LOG_FILE=$SUITE_RESULTS_DIRECTORY/memory.log
MEMORY_INDEX_FILE=$SUITE_RESULTS_DIRECTORY/memory-index.log

# Clean entrance into the script.
start_process

save_current_directory

clear_suite_output

start_database_if_required

if [ $TRACK_MEMORY -ne 0 ] ; then
    "$SCRIPTPATH/python/scan_memory.py"  > "$MEMORY_LOG_FILE" 2>&1 &
    # shellcheck disable=SC2181
    if [ $? -ne 0 ] ; then
        cat "$MEMORY_LOG_FILE"
        complete_process 1 "Cannot start the database memory monitoring."
    fi
fi

install_and_build_cleanly

broadcast_message "$SUITE_MODE" "Testing of the test suite started."
DID_REPORT_START=1

# shellcheck disable=SC2016
IFS=$'\r\n' GLOBIGNORE='*' command eval  'TEST_NAMES=($(cat $SUITE_FILE_NAME))'
for NEXT_TEST_NAME in "${TEST_NAMES[@]}"; do
    execute_suite_test "$NEXT_TEST_NAME"
done

kill_database_if_was_started

if [ $TRACK_MEMORY -ne 0 ] ; then
    echo "" >> "$MEMORY_INDEX_FILE"
    if ! "$SCRIPTPATH/python/summarize_memory_logs.py" --log-file "$MEMORY_LOG_FILE" --index-file "$MEMORY_INDEX_FILE" --output "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/memory.json" ; then
        complete_process 1 "Summarizing the memory logs failed."
    fi
fi

broadcast_message "$SUITE_MODE" "Testing of the test suite completed.  Summarizing suite results."
if ! "$SCRIPTPATH/python/summarize_suite_results.py" "$SUITE_FILE_NAME"; then
    complete_process 1 "Summarizing the results failed."
fi

if [ $SHOW_STATS -ne 0 ] ; then
    OUTPUT_FILE="$TEMP_FILE"
    if ! "$SCRIPTPATH/python/summary_stats.py" > $OUTPUT_FILE 2>&1 ; then
        cat $OUTPUT_FILE
        complete_process 1 "Displaying statistics for the suite failed."
    fi
    if [ $SHOW_JSON_STATS -ne 0 ] ; then
        OUTPUT_FILE=$SUITE_RESULTS_DIRECTORY/suite-stats.json
        WORKLOAD_PARAMETER=
        if [ -n "$WORKLOAD_NAME" ] ; then
            WORKLOAD_PARAMETER="--workload $WORKLOAD_NAME"
        fi
        # shellcheck disable=SC2086
        if ! $SCRIPTPATH/python/summary_stats.py --format json $WORKLOAD_PARAMETER > $OUTPUT_FILE 2>&1 ; then
            cat $OUTPUT_FILE
            complete_process 1 "Displaying statistics for the suite failed."
        fi
    fi
    if [ $SHOW_JSON_STATS -eq 0 ] ; then
        cat $OUTPUT_FILE
    fi
fi

if [ $SLACK_MODE -ne 0 ] ; then
    "$SCRIPTPATH/python/publish_to_slack.py attachment" "$SCRIPTPATH/$SUITE_RESULTS_DIRECTORY/summary.json" "application/json"
fi

# If we get here, we have a clean exit from the script.
complete_process 0
