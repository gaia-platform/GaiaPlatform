# shellcheck shell=bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# ----------------------------------------------------
# Variables that are specific to this project.
# ----------------------------------------------------

# Name of the project.  Safe to include in filenames, etc.
export PROJECT_NAME="incubator"

# Name of the project's executable within the build directory.
export EXECUTABLE_NAME="incubator"

# Name of the project's database.
export DATABASE_NAME="incubator"

# Various parts that are built to generate the executable.
# Used for allowing an "autobuild" option on run.sh.
export RULESET_PATHS=("incubator.ruleset")
export DDL_PATHS=("incubator.ddl")
export SOURCE_PATHS=("incubator.cpp")

# ----------------------------------------------------
# Variables that are general to this group of scripts.
# ----------------------------------------------------

# Relative directory where the build results are stored.
export BUILD_DIRECTORY="build"

# Relative directory where the logs are stored.
export LOG_DIRECTORY="logs"

# Relative directory where the test results are stored.
export TEST_RESULTS_DIRECTORY="test-results"

# Relative directory where the test suite results are stored.
export SUITE_RESULTS_DIRECTORY="suite-results"

# Absolute temp directory where the project is installed to and operated
# on during the testing process.
export TEST_DIRECTORY=/tmp/test_$PROJECT_NAME

# -----------------------------------------------------------------
# Functions to specify how to process the command line with run.sh.
#
# This is placed in this file to make the core scripts as agnostic
# as possible for reuse.
# -----------------------------------------------------------------

export TEST_COMMAND_NAME="debug"

# Process a debug command which executes a file containing commands
# and captures any output.  Normal assumption is that the output is
# in the form of a JSON file.
process_debug() {
    local DEBUG_COMMAND_FILE=$1
    local SCRIPT_STOP_PAUSE=$2

    JSON_OUTPUT=$BUILD_DIRECTORY/output.json
    CSV_OUTPUT=$BUILD_DIRECTORY/output.csv
    STOP_OUTPUT=$BUILD_DIRECTORY/output.delay

    if [ -z "$DEBUG_COMMAND_FILE" ]
    then
        echo "No debug file to execute supplied for command 'debug'."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the executable $EXECUTABLE_NAME in debug mode with input file: $(realpath "$DEBUG_COMMAND_FILE")"
    fi

    # Run the commands and produce a JSON output file.
    if ! "$EXECUTABLE_PATH" debug "$SCRIPT_STOP_PAUSE" < "$DEBUG_COMMAND_FILE" > "$JSON_OUTPUT"; then
        cat $JSON_OUTPUT
        echo "Execution of the executable $EXECUTABLE_PATH in debug mode failed."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "JSON output file located at: $(realpath "$JSON_OUTPUT")"
    fi

    # For ease of graphing, also produce a CSV file if requested.
    if [ "$GENERATE_CSV_MODE" -ne 0 ]; then
        lines_in_output_json=$(grep "" -c $JSON_OUTPUT)
        if [ "$lines_in_output_json" -eq 1 ] ; then
            echo "Translation of the JSON output to CSV skipped. Nothing to translate."
        else
            head -n -1 $JSON_OUTPUT > $TEST_DIRECTORY/blah
            if ! ./python/translate_to_csv.py "$TEST_DIRECTORY/blah" > "$CSV_OUTPUT"; then
                echo "Translation of the JSON output to CSV failed."
                complete_process 1
            fi
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "CSV output file located at: $(realpath "$CSV_OUTPUT")"
            fi
        fi
    fi

    # Assuming that the last line of the file has a prefix of `stop_pause_in_sec:`,
    # remove it and locate it elsewhere in that folder.
    tail -n 1 $JSON_OUTPUT > $STOP_OUTPUT
    head -n -1 $JSON_OUTPUT > $TEST_DIRECTORY/blah
    cp $TEST_DIRECTORY/blah $JSON_OUTPUT
}

# Process one of the commands to watch the changes of the database
# on a second by second interval.
process_watch() {
    watch -n 1 "$EXECUTABLE_PATH" "$1"
}

# Process a normal command that interacts with the user.
process_normal() {
    if ! "$EXECUTABLE_PATH" "$1"; then
        echo "Execution of the executable $EXECUTABLE_NAME in $1 mode failed."
        complete_process 1
    fi
}

# Process the various commands.
execute_commands() {
    GENERATE_CSV_MODE=$1
    DEBUG_END_PAUSE=$2

    if [[ "${PARAMS[0]}" == "$TEST_COMMAND_NAME" ]]; then
        process_debug "${PARAMS[1]}" "$DEBUG_END_PAUSE"
    elif [[ "${PARAMS[0]}" == "watch" ]]; then
        process_watch "show"
    elif [[ "${PARAMS[0]}" == "watch-json" ]]; then
        process_watch "showj"
    elif [[ "${PARAMS[0]}" == "show" ]]; then
        process_normal "show"
    elif [[ "${PARAMS[0]}" == "show-json" ]]; then
        process_normal "showj"
    elif [[ "${PARAMS[0]}" == "run" ]]; then
        process_normal "sim"
    elif [[ "${PARAMS[0]}" == "" ]]; then
        echo "Command was not provided."
        show_usage
    else
        echo "Command '${PARAMS[0]}' not known."
        complete_process 1
    fi
}

show_usage_commands() {
    echo "Commands:"
    echo "  run               Run the simulator in normal mode."
    echo "  debug <file>      Debug the simulator using the specified file."
    echo "  show              Show the current state of the simulation and exit."
    echo "  show-json         Show the current state of the simulation as JSON and exit."
    echo "  watch             Show the state of the simulation every second."
    echo "  watch-json        Show the state of the simulation as JSON every second."
}
