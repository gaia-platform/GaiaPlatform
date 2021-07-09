#! /usr/bin/bash

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the $PROJECT_NAME project..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    # $1 is the return code to assign to the script
    if [ "$1" -ne 0 ]; then
        echo "Execution of the $PROJECT_NAME project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Execution of the $PROJECT_NAME project succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi

    exit "$1"
}

# Show how this script can be used.
show_usage() {
    echo "Usage: $(basename "$0") [flags] <command>"
    echo "Flags:"
    echo "  -a,--auto         Automatically build the project before execution, if needed."
    echo "  -c,--csv          Generate a CSV output file if applicable."
    echo "  -v,--verbose      Show lots of information while executing the project."
    echo "  -h,--help         Display this help text."
    echo ""
    echo "Commands:"
    echo "  run               Run the simulator in normal mode."
    echo "  debug <file>      Debug the simulator using the specified file."
    echo "  show              Show the current state of the simulation and exit."
    echo "  show-json         Show the current state of the simulation as JSON and exit."
    echo "  watch             Show the state of the simulation every second."
    echo "  watch-json        Show the state of the simulation as JSON every second."
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    AUTO_BUILD_MODE=0
    GENERATE_CSV_MODE=0
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
}

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
                echo "Test script cannot build the project in directory '$(realpath "$TEST_DIRECTORY")'."
                complete_process 1
            fi
        else
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Not autobuilding the $PROJECT_NAME project.  All dependent files up to date."
            fi
        fi
    fi
}

# Process a debug command which executes a file containing commands
# and captures any output.  Normal assumption is that the output is
# in the form of a JSON file.
process_debug() {

    if [ -z "$1" ]
    then
        echo "No debug file to execute supplied for command 'debug'."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the executable $EXECUTABLE_NAME in debug mode with input file: $(realpath "$1")"
    fi

    # Run the commands and produce a JSON output file.
    if ! "$EXECUTABLE_PATH" debug < "$1" > "$JSON_OUTPUT"; then
        echo "Execution of the executable $EXECUTABLE_PATH in debug mode failed."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "JSON output file located at: $(realpath "$JSON_OUTPUT")"
    fi

    # For ease of graphing, also produce a CSV file if requested.
    if [ "$GENERATE_CSV_MODE" -ne 0 ]; then
        if ! ./translate_to_csv.py > "$CSV_OUTPUT"; then
            echo "Translation of the JSON output to CSV failed."
            complete_process 1
        fi
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "CSV output file located at: $(realpath "$CSV_OUTPUT")"
        fi
    fi
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



# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091
source "$SCRIPTPATH/properties.sh"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.run.tmp

# Set up any local script variables.
JSON_OUTPUT=$BUILD_DIRECTORY/output.json
CSV_OUTPUT=$BUILD_DIRECTORY/output.csv
EXECUTABLE_PATH=./$BUILD_DIRECTORY/$EXECUTABLE_NAME


# Parse any command line values.
parse_command_line "$@"

# Handle the auto-build functionality.
handle_auto_build

# Make sure the program is compiled before going on.
if [ ! -f "$EXECUTABLE_PATH" ]; then
    echo "Building of the project has not be completed.  Cannot run."
    complete_process 1
fi

# Clean entrance into the script.
start_process

# Process the various commands.
if [[ "${PARAMS[0]}" == "debug" ]]; then
    process_debug "${PARAMS[1]}"
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

# If we get here, we have a clean exit from the script.
complete_process 0 "$1"

