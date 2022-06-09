#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the $PROJECT_NAME project..."
    fi
}

# Simple function to stop the process, including any cleanup.
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Execution of the $PROJECT_NAME project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Execution of the $PROJECT_NAME project succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    exit "$SCRIPT_RETURN_CODE"
}

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags] <command>"
    echo "Flags:"
    echo "  -a,--auto                   Automatically build the project before execution, if needed."
    echo "  -c,--csv                    Generate a CSV output file if applicable."
    echo "  -d,--directory              Directory to run the tests against."
    echo "  -g,--config <file>          Generate a configuration file specific to this run."
    echo "  -nt,--num-threads <threads> Number of threads to use for the rule engine. If specified,"
    echo "                              this overrides any configuration in a specified configuration"
    echo "                              file. If '0' is specified, then the number of threads is set"
    echo "                              to maximum."
    echo "  -v,--verbose                Display detailed information during execution."
    echo "  -h,--help                   Display this help text."
    echo ""
    show_usage_commands
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    AUTO_BUILD_MODE=0
    GENERATE_CSV_MODE=0
    CONFIG_FILE=
    NUMBER_OF_THREADS=-1
    ALTERNATE_TEST_DIRECTORY=
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -a|--auto)
            AUTO_BUILD_MODE=1
            shift
        ;;
        -c|--csv)
            GENERATE_CSV_MODE=1
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
        -g|--config)
            # shellcheck disable=SC2086
            if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
                CONFIG_FILE=$2
                shift 2
            else
                echo "Error: Argument for $1 is missing" >&2
                exit 1
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

    if [ -n "$ALTERNATE_TEST_DIRECTORY" ] ; then
        TEST_DIRECTORY=$ALTERNATE_TEST_DIRECTORY
    fi
}

# Check to see if an individual component to make the executable
# has been updated since the last time the executable was built.
check_auto_build_component() {
    local NEXT_SOURCE_ITEM=$1
    local SOURCE_ITEM_TYPE=$2
    local SET_FULL_FLAG_IF_CHANGED=$3

    if [[ "$NEXT_SOURCE_ITEM" -nt "$EXECUTABLE_PATH" ]]; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "$SOURCE_ITEM_TYPE file $NEXT_SOURCE_ITEM has changed."
        fi
        DO_BUILD=1
        if [ "$SET_FULL_FLAG_IF_CHANGED" -ne 0 ]; then
            FULL_BUILD_FLAG=-f
        fi
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "$SOURCE_ITEM_TYPE file $NEXT_SOURCE_ITEM has not changed."
        fi
    fi
}

# Check to see if any of our core files have changed, and automatically
# build if that happens.
handle_auto_build() {
    if [ $AUTO_BUILD_MODE -ne 0 ]; then
        DO_BUILD=0
        FULL_BUILD_FLAG=
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Checking for any out-of-sync dependent files needed to build $EXECUTABLE_NAME."
        fi
        if [ ! -f "$EXECUTABLE_PATH" ]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Executable $EXECUTABLE_NAME does not exist."
            fi
            DO_BUILD=1
        else
            for NEXT_SOURCE_ITEM in "${RULESET_PATHS[@]}"; do
                check_auto_build_component "$NEXT_SOURCE_ITEM" "Ruleset" 1
            done
            for NEXT_SOURCE_ITEM in "${DDL_PATHS[@]}"; do
                check_auto_build_component "$NEXT_SOURCE_ITEM" "DDL" 1
            done
            for NEXT_SOURCE_ITEM in "${SOURCE_PATHS[@]}"; do
                check_auto_build_component "$NEXT_SOURCE_ITEM" "Source" 0
            done
        fi

        if [ $DO_BUILD -ne 0 ]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Autobuilding the $PROJECT_NAME project."
            fi
            if ! ./build.sh -v "$FULL_BUILD_FLAG" > "$TEMP_FILE" 2>&1 ; then
                cat "$TEMP_FILE"
                complete_process 1 "Build script cannot build the project in directory '$(realpath "$TEST_DIRECTORY")'."
            fi
        else
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Not autobuilding the $PROJECT_NAME project.  All dependent files up to date."
            fi
        fi
    fi
}

# Create a configuration file from the config.json file in the project's
# test file hierarchy.
create_configuration_file() {

    local CONFIG_PATH=
    CONFIGURATION_PATH=$(realpath "$GENERATED_CONFIGURATION_FILE")

    THREADS_ARGUMENT=
    if [ "$NUMBER_OF_THREADS" -ge 0 ] ; then
        THREADS_ARGUMENT="--threads $NUMBER_OF_THREADS"
    fi

    if [ -z "$CONFIG_FILE" ]; then
        echo "No configuration file specified.  Generating gaia configuation file with default values."
        # shellcheck disable=SC2086
        ./python/generate_config.py --output $CONFIGURATION_PATH $THREADS_ARGUMENT > "$TEMP_FILE" 2>&1
        DID_FAIL=$?
    else
        CONFIG_PATH=$(realpath "$CONFIG_FILE")
        echo "Configuration file '$CONFIG_PATH' specified.  Generating gaia configuration file."
        # shellcheck disable=SC2086
        ./python/generate_config.py --output $CONFIGURATION_PATH $THREADS_ARGUMENT --config "$CONFIG_PATH"  > "$TEMP_FILE" 2>&1
        DID_FAIL=$?
    fi

    if [ $DID_FAIL -ne 0 ]; then
        cat "$TEMP_FILE"
        complete_process 1 "Generating gaia configuration file '$CONFIGURATION_PATH' failed."
    else
        echo "Gaia configuration file '$CONFIGURATION_PATH' generated."
        STATS_LOG_INTERVAL=$(cat "$TEMP_FILE")
    fi
}

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091 source=./properties.sh
source "$SCRIPTPATH/properties.sh"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.run.tmp

# Parse any command line values.
parse_command_line "$@"

# Handle the auto-build functionality.
handle_auto_build

# Create the configuration file specific to gaia from a simple JSON file.
create_configuration_file

# Make sure the program is compiled before going on.
if [ ! -f "$EXECUTABLE_PATH" ]; then
    complete_process 1 "Building of the project has not be completed.  Cannot run."
fi

# Make sure that the program configuration file is present.
if [ ! -f "$CONFIGURATION_PATH" ]; then
    complete_process 1 "Building of the project configuration file has not be completed.  Cannot run."
fi

# Clean entrance into the script.
start_process

execute_commands "$GENERATE_CSV_MODE" $((STATS_LOG_INTERVAL*2))

# If we get here, we have a clean exit from the script.
complete_process 0
