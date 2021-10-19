# shellcheck shell=bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# ----------------------------------------------------
# Variables that are specific to this project.
# ----------------------------------------------------

# Name of the project.  Safe to include in filenames, etc.
export PROJECT_NAME="pingpong"

# Name of the project's executable within the build directory.
export EXECUTABLE_NAME="pingpong"

# Name of the project's database.
export DATABASE_NAME="pingpong"

# Various parts that are built to generate the executable.
# Used for allowing an "autobuild" option on run.sh.
export RULESET_PATHS=("pingpong.ruleset")
export DDL_PATHS=("pingpong.ddl")
export SOURCE_PATHS=("pingpong.cpp")

export GENERATED_CONFIGURATION_FILE=("pingpong.conf")

export TEMPLATE_PATH=("$SCRIPTPATH/template")

# ----------------------------------------------------
# Variables that are general to this group of scripts.
# ----------------------------------------------------

# Relative directory where the build results are stored.
export BUILD_DIRECTORY="build"

# Relative directory where the logs are stored.
export LOG_DIRECTORY="logs"

# Relative directory where the test results are stored.
export TEST_RESULTS_DIRECTORY="test-results"

# Absolute temp directory where the project is installed to and operated
# on during the testing process.
export TEST_DIRECTORY=/tmp/test_$PROJECT_NAME

# Set up any local script variables.
export EXECUTABLE_PATH=./$BUILD_DIRECTORY/$EXECUTABLE_NAME


# -----------------------------------------------------------------
# Functions to specify project specific information for test.sh.
#
# This is placed in this file to make the core scripts as agnostic
# as possible for reuse.
# -----------------------------------------------------------------
copy_extra_test_files() {
    echo "No extra files to copy."
}

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

    EXECUTE_OUTPUT=$TEST_RESULTS_DIRECTORY/execute.json

    if [ -z "$DEBUG_COMMAND_FILE" ]
    then
        complete_process 1 "No debug file to execute supplied for command 'debug'."
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the executable $EXECUTABLE_NAME in debug mode with input file: $(realpath "$DEBUG_COMMAND_FILE")"
    fi

    # Run the commands and produce a JSON output file.
    if ! "$EXECUTABLE_PATH" debug "$SCRIPT_STOP_PAUSE" < "$DEBUG_COMMAND_FILE" > "$EXECUTE_OUTPUT"; then
        cat $EXECUTE_OUTPUT
        complete_process 1 "Execution of the executable $EXECUTABLE_PATH in debug mode failed."
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executable output file located at: $(realpath "$EXECUTE_OUTPUT")"
    fi
}

# Process the various commands.
execute_commands() {
    DEBUG_END_PAUSE=$2

    if [[ "${PARAMS[0]}" == "$TEST_COMMAND_NAME" ]]; then
        process_debug "${PARAMS[1]}" "$DEBUG_END_PAUSE"
    elif [[ "${PARAMS[0]}" == "" ]]; then
        echo "Command was not provided."
        show_usage
    else
        complete_process 1 "Command '${PARAMS[0]}' not known."
    fi
}

show_usage_commands() {
    echo "Commands:"
}
