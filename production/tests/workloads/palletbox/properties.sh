#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

# ----------------------------------------------------
# Variables that are specific to this project.
# ----------------------------------------------------

# Name of the project.  Safe to include in filenames, etc.
export PROJECT_NAME="palletbox"

# Name of the project's executable within the build directory.
export EXECUTABLE_NAME="palletbox"

# Name of the project's database.
export DATABASE_NAME="palletbox"

# Various parts that are built to generate the executable.
# Used for allowing an "autobuild" option on run.sh.
export RULESET_PATHS=("palletbox.ruleset")
export DDL_PATHS=("palletbox.ddl")
export SOURCE_PATHS=("palletbox.cpp")

export GENERATED_CONFIGURATION_FILE=("palletbox.conf")

export TEMPLATE_PATH="$SCRIPTPATH/../template"

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
# Functions to specify project specific information for build.sh.
#
# This is placed in this file to make the core scripts as agnostic
# as possible for reuse.
# -----------------------------------------------------------------

# With everything else set up, do the heavy lifting of building the project.
build_project() {
    generate_makefile
    invoke_makefile
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

# Process a debug command which executes a file containing commands
# and captures any output.  Normal assumption is that the output is
# in the form of a JSON file.
process_debug() {
    local COMMAND_FILE=$1
    local SCRIPT_STOP_PAUSE=$2

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the executable $EXECUTABLE_PATH in debug mode with input file: $(realpath "$COMMAND_FILE")"
    fi

    echo "---"
    echo "Application Log"
    echo "---"

    # Run the commands and produce a JSON output file.
    if ! "$EXECUTABLE_PATH" debug "$SCRIPT_STOP_PAUSE" < "$COMMAND_FILE" > "$JSON_OUTPUT"; then
        cat "$JSON_OUTPUT"
        complete_process 1 "Execution of the executable $EXECUTABLE_PATH in debug mode failed."
    fi

    tail -n 1 "$JSON_OUTPUT" > "$STOP_OUTPUT"
    head -n -1 "$JSON_OUTPUT" > $TEST_DIRECTORY/blah
    cp $TEST_DIRECTORY/blah "$JSON_OUTPUT"

    echo "---"
    echo "Application Standard Out"
    echo "---"
    cat "$JSON_OUTPUT"
    echo "---"
}

# Process a debug command which executes a file containing commands
# and captures any output.  Normal assumption is that the output is
# in the form of a JSON file.
execute_commands() {
    DEBUG_END_PAUSE=$2

    JSON_OUTPUT=$BUILD_DIRECTORY/output.json
    STOP_OUTPUT=$BUILD_DIRECTORY/output.delay

    if [[ "${PARAMS[0]}" == "$TEST_COMMAND_NAME" ]]; then
        process_debug "${PARAMS[1]}" "$DEBUG_END_PAUSE"
    elif [[ "${PARAMS[0]}" == "" ]]; then
        echo "Command was not provided."
        show_usage
    else
        complete_process 1 "Command '${PARAMS[0]}' not known."
    fi
}
