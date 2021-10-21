# shellcheck shell=bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# ----------------------------------------------------
# Variables that are specific to this project.
# ----------------------------------------------------

# Name of the project.  Safe to include in filenames, etc.
export PROJECT_NAME="template"

# Name of the project's executable within the build directory.
export EXECUTABLE_NAME="template"

# Name of the project's database.
export DATABASE_NAME="template"

# Various parts that are built to generate the executable.
# Used for allowing an "autobuild" option on run.sh.
export RULESET_PATHS=("template.ruleset")
export DDL_PATHS=("template.ddl")
export SOURCE_PATHS=("template.cpp")

export GENERATED_CONFIGURATION_FILE=("template.conf")

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
    echo ""
}

# -----------------------------------------------------------------
# Functions to specify how to process the command line with run.sh.
#
# This is placed in this file to make the core scripts as agnostic
# as possible for reuse.
# -----------------------------------------------------------------

export TEST_COMMAND_NAME="debug"

# Any additional information to show about commands.
show_usage_commands() {
    echo ""
    echo "  $TEST_COMMAND_NAME               Run the simulation in debug mode."
}

# Handle the processing of the debug command.
process_debug() {
    COMMAND_FILE=$1

    echo "execute variables '$GENERATE_CSV_MODE' '$DEBUG_END_PAUSE'"
    echo "execute input file '$COMMAND_FILE'"

    # Do something similar to...
    #
    # if ! "$EXECUTABLE_PATH" debug "$SCRIPT_STOP_PAUSE" < "$DEBUG_COMMAND_FILE" > "$JSON_OUTPUT"; then
    #     cat $JSON_OUTPUT
    #     echo "Execution of the executable $EXECUTABLE_PATH in debug mode failed."
    #     complete_process 1
    # fi

    mkdir $LOG_DIRECTORY
    touch $LOG_DIRECTORY/gaia.log
    touch $LOG_DIRECTORY/gaia_stats.log
}

# Process a debug command which executes a file containing commands
# and captures any output.  Normal assumption is that the output is
# in the form of a JSON file.
# Process the various commands.
execute_commands() {
    GENERATE_CSV_MODE=$1
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
