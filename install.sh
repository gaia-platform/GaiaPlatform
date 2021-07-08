#! /usr/bin/bash

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing the incubator example to directory $1"
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    # $1 is the return code to assign to the script
    if [ "$1" -ne 0 ]; then
        echo "Installation of the incubator example failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Installation of the incubator example succeeded."
        fi
    fi
    exit "$1"
}

# Show how this script can be used.
show_usage() {
    echo "Usage: $(basename "$0") [flags] <directory>"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while installing the example."
    echo "  -h,--help         Display this help text."
    echo "Other:"
    echo "  directory         Directory to create and install the example into."
    exit 1
}

# Parse any command line values.
parse_command_line() {
    VERBOSE_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -h|--help) # unsupported flags
        show_usage
        ;;
        -v|--verbose)
        VERBOSE_MODE=1
        shift
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

# Calculate where to install the project to and that it is in the right state
# for that install to continue.
calculate_install_directory() {

    if [ -z "${PARAMS[0]}" ]
    then
        echo "Directory to install the example into was not provided."
        show_usage
    fi
    INSTALL_DIRECTORY=${PARAMS[0]}
    length=${#INSTALL_DIRECTORY}
    last_char=${INSTALL_DIRECTORY:length-1:1}
    [[ $last_char != "/" ]] && INSTALL_DIRECTORY="$INSTALL_DIRECTORY/"; :

    # Make sure it does not already exist.
    if [ -d "$INSTALL_DIRECTORY" ]; then
        echo "Directory '$(realpath "$INSTALL_DIRECTORY")' already exists. Please specify a new directory to create."
        complete_process 1
    fi

    if [ -f "$INSTALL_DIRECTORY" ]; then
        echo "'$(realpath "$INSTALL_DIRECTORY")' already exists as a file. Please specify a new directory to create."
        complete_process 1
    fi
}

# Copy the example into the directory.
install_into_directory() {
    if ! cp -rf "$SCRIPTPATH" "$INSTALL_DIRECTORY"; then
        echo "Example was not copied into the '$(realpath "$INSTALL_DIRECTORY")' directory."
        complete_process 1
    fi
    if [ -d "$INSTALL_DIRECTORY/build" ]; then
        if ! rm -rf "$INSTALL_DIRECTORY/build"; then
            echo "Existing '$(realpath "$INSTALL_DIRECTORY/build")' directory not removed from the '$(realpath "$INSTALL_DIRECTORY")' directory."
            complete_process 1
        fi
    fi
    if [ -d "$INSTALL_DIRECTORY/logs" ]; then
        if ! rm -rf "$INSTALL_DIRECTORY/logs"; then
            echo "Existing '$(realpath "$INSTALL_DIRECTORY/logs")' directory not removed from the '$(realpath "$INSTALL_DIRECTORY")' directory."
            complete_process 1
        fi
    fi
    if [ -d "$INSTALL_DIRECTORY/test-results" ]; then
        if ! rm -rf "$INSTALL_DIRECTORY/test-results"; then
            echo "Existing '$(realpath "$INSTALL_DIRECTORY/test-results")' directory not removed from the '$(realpath "$INSTALL_DIRECTORY")' directory."
            complete_process 1
        fi
    fi

    # ...then go into that directory.
    if ! cd "$INSTALL_DIRECTORY"; then
        echo "Cannot change the current directory to the '$(realpath "$INSTALL_DIRECTORY")' directory."
        complete_process 1
    fi
}

# Set up any script variables.
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

parse_command_line "$@"

calculate_install_directory

# Clean entrance into the script.
start_process "$INSTALL_DIRECTORY"

install_into_directory

# If we get here, we have a clean exit from the script.
complete_process 0 "$1"
