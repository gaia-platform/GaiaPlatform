#! /usr/bin/bash

start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing the incubator example to directory $1"
    fi
}

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

show_usage() {
    echo "Usage: $(basename "$0") [flags] <directory>"
    echo "Flags:"
    echo "  -v,--verbose      Show lots of information while installing the example."
    echo "  -h,--help         Display this help text."
    echo "Other:"
    echo "  directory         Directory to create and install the example into."
    exit 1
}

# Set up any script variables.
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Parse any command line values.
VERBOSE_MODE=0
PARAMS=""
while (( "$#" )); do
  case "$1" in
    # -b|--my-flag-with-argument)
    #   if [ -n "$2" ] && [ ${2:0:1} != "-" ]; then
    #     MY_FLAG_ARG=$2
    #     shift 2
    #   else
    #     echo "Error: Argument for $1 is missing" >&2
    #     exit 1
    #   fi
    #   ;;
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
      PARAMS="$PARAMS $1"
      shift
      ;;
  esac
done
eval set -- "$PARAMS"


if [ -z "$1" ]
then
    echo "Directory to install the example into was not provided."
    show_usage
fi
INSTALL_DIRECTORY=$1
length=${#INSTALL_DIRECTORY}
last_char=${INSTALL_DIRECTORY:length-1:1}
[[ $last_char != "/" ]] && INSTALL_DIRECTORY="$INSTALL_DIRECTORY/"; :

# Make sure it does not already exist.
if [ -d "$INSTALL_DIRECTORY" ]; then
    echo "Directory $INSTALL_DIRECTORY already exists. Please specify a new directory to create."
    complete_process 1
fi

if [ -f "$INSTALL_DIRECTORY" ]; then
    echo "File $INSTALL_DIRECTORY already exists. Please specify a new directory to create."
    complete_process 1
fi

# Clean entrance into the script.
start_process "$INSTALL_DIRECTORY"

# Copy the example into a new directory,
if ! cp -rf "$SCRIPTPATH" "$INSTALL_DIRECTORY"; then
    echo "Example was not copied into the $INSTALL_DIRECTORY directory."
    complete_process 1
fi
if ! rm -rf "$INSTALL_DIRECTORY/build"; then
    echo "Possibly existing 'build' directory not removed from the $INSTALL_DIRECTORY directory."
    complete_process 1
fi
if ! rm -rf "$INSTALL_DIRECTORY/logs"; then
    echo "Possibly existing 'logs' directory not removed from the $INSTALL_DIRECTORY directory."
    complete_process 1
fi

# ...then go into that directory.
if ! cd "$INSTALL_DIRECTORY"; then
    echo "Cannot change the current directory to the $INSTALL_DIRECTORY directory."
    complete_process 1
fi

# If we get here, we have a clean exit from the script.
complete_process 0 "$1"
