#! /usr/bin/bash

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing the $PROJECT_NAME project to directory $1"
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    # $1 is the return code to assign to the script
    if [ "$1" -ne 0 ]; then
        echo "Installation of the $PROJECT_NAME project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Installation of the $PROJECT_NAME project succeeded."
        fi
    fi
    exit "$1"
}

# Show how this script can be used.
show_usage() {
    echo "Usage: $(basename "$0") [flags] <directory>"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while installing the project."
    echo "  -h,--help         Display this help text."
    echo "Other:"
    echo "  directory         Directory to create and install the project into."
    exit 1
}

# Parse any command line values.
parse_command_line() {
    VERBOSE_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
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

# Calculate where to install the project to and that it is in the right state
# for that install to continue.
calculate_install_directory() {

    if [ -z "${PARAMS[0]}" ]
    then
        echo "Directory to install the project into was not provided."
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

# Remove any dynamic directories so that the build is really clean.
remove_dynamic_directory() {

    if [ "$1" == "/" ]; then
        echo "Removing the specified directory '$1' is dangerous. Aborting."
        complete_process 1
    fi

    if [ -d "$1" ]; then
        if ! rm -rf "$1"; then
            echo "Existing '$(realpath "$1")' directory not removed from the '$(realpath "$INSTALL_DIRECTORY")' directory."
            complete_process 1
        fi
    fi
}

# Copy the example into the directory.
install_into_directory() {
    if ! cp -rf "$SCRIPTPATH" "$INSTALL_DIRECTORY"; then
        echo "Project $PROJECT_NAME cannot be copied into the '$(realpath "$INSTALL_DIRECTORY")' directory."
        complete_process 1
    fi
    remove_dynamic_directory "$INSTALL_DIRECTORY/$BUILD_DIRECTORY"
    remove_dynamic_directory "$INSTALL_DIRECTORY/$LOG_DIRECTORY"
    remove_dynamic_directory "$INSTALL_DIRECTORY/$TEST_RESULTS_DIRECTORY"

    # ...then go into that directory.
    if ! cd "$INSTALL_DIRECTORY"; then
        echo "Cannot change the current directory to the '$(realpath "$INSTALL_DIRECTORY")' directory."
        complete_process 1
    fi
}



# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091
source "$SCRIPTPATH/properties.sh"

# Set up any project based local script variables.

# Set up any local script variables.



parse_command_line "$@"

calculate_install_directory

# Clean entrance into the script.
start_process "$INSTALL_DIRECTORY"

install_into_directory

# If we get here, we have a clean exit from the script.
complete_process 0 "$1"
