#! /usr/bin/bash

start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the incubator..."
    fi
}

complete_process() {
    # $1 is the return code to assign to the script
    if [ "$1" -ne 0 ]; then
        echo "Execution of the incubator failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Execution of the incubator succeeded."
        fi
    fi
    exit "$1"
}

show_usage() {
    echo "Usage: $(basename "$0") [flags] <command>"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while executing the project."
    echo "  -h,--help         Display this help text."
    echo "  -a,--auto         Automatically build the project before execution, if needed."
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

# Set up any script variables.
JSON_OUTPUT=build/output.json
CSV_OUTPUT=build/output.csv
TEMP_FILE=/tmp/incubator.run.tmp

# Parse any command line values.
VERBOSE_MODE=0
AUTO_BUILD_MODE=0
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
    -a|--auto)
      AUTO_BUILD_MODE=1
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

handle_auto_build() {
    if [ $AUTO_BUILD_MODE -ne 0 ]; then
        DO_BUILD=0
        FULL_BUILD_FLAG=
        MY_EXECUTABLE="build/incubator"
        MY_CPP_FILE="incubator.cpp"
        MY_RULESET_FILE="incubator.ruleset"
        MY_DDL_FILE="incubator.ddl"
        if [ ! -f "$MY_EXECUTABLE" ]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Incubator executable does not exist."
            fi
            DO_BUILD=1
        elif [[ "$MY_RULESET_FILE" -nt "$MY_EXECUTABLE" ]]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Ruleset file $MY_RULESET_FILE has changed."
            fi
            DO_BUILD=1
            FULL_BUILD_FLAG=-f
        elif [[ "$MY_DDL_FILE" -nt "$MY_EXECUTABLE" ]]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "DDL file $MY_DDL_FILE has changed."
            fi
            DO_BUILD=1
            FULL_BUILD_FLAG=-f
        elif [[ "$MY_CPP_FILE" -nt "$MY_EXECUTABLE" ]]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Source file $MY_CPP_FILE has changed."
            fi
            DO_BUILD=1
        fi

        if [ $DO_BUILD -ne 0 ]; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Autobuilding the incubator project."
            fi
            if ! ./build.sh -v $FULL_BUILD_FLAG >$TEMP_FILE 2>&1 ; then
                cat $TEMP_FILE
                echo "Test script cannot build the project in directory '$TEST_DIRECTORY'."
                complete_process 1
            fi
        fi
    fi
}

process_debug() {

    if [ -z "$1" ]
    then
        echo "No debug file to execute supplied for command 'debug'."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the incubator in debug mode with input file: $(realpath "$1")"
    fi

    # Run the commands and produce a JSON output file.
    if ! ./build/incubator debug < "$1" > $JSON_OUTPUT; then
        echo "Execution of the incubator failed."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "JSON output file located at: $(realpath $JSON_OUTPUT)"
    fi

    # For ease of graphing, also produce a CSV file.
    if ! ./translate_to_csv.py > $CSV_OUTPUT; then
        echo "Translation of the JSON output to CSV failed."
        complete_process 1
    fi
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "CSV output file located at: $(realpath $CSV_OUTPUT)"
    fi
}

process_watch() {

    watch -n 1 ./build/incubator "$1"
}

process_normal() {

    if ! ./build/incubator "$1"; then
        echo "Execution of the incubator failed."
        complete_process 1
    fi
}

# Handle the auto-build functionality.
echo "auto"
handle_auto_build

# Make sure the program is compiled before going on.
if [ ! -f "build/incubator" ]; then
    echo "Execution of the program has not be completed.  Cannot run."
    complete_process 1
fi

# Clean entrance into the script.
start_process

# Process the various commands.
if [[ "$1" == "debug" ]]; then
    process_debug "$2"
elif [[ "$1" == "watch" ]]; then
    process_watch "show"
elif [[ "$1" == "watch-json" ]]; then
    process_watch "showj"
elif [[ "$1" == "show" ]]; then
    process_normal "show"
elif [[ "$1" == "show-json" ]]; then
    process_normal "showj"
elif [[ "$1" == "run" ]]; then
    process_normal "sim"
elif [[ "$1" == "" ]]; then
    echo "Command was not provided."
    show_usage
else
    echo "Command \'$1\' not known."
    complete_process 1
fi

# If we get here, we have a clean exit from the script.
complete_process 0 "$1"
