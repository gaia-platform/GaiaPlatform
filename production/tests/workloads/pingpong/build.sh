#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################


# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo Building the project "$PROJECT_NAME"...
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Build of project $PROJECT_NAME failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Build of project $PROJECT_NAME succeeded."
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

    echo "Usage: $(basename "$SCRIPT_NAME") [flags]"
    echo "Flags:"
    echo "  -f,--force        Force a complete build of the project."
    echo "  -l,--lint         In addition to building the project, lint it as well."
    echo "  -m,--make-only    Build only the make part of the project."
    echo "  -nc,--no-cache    (Experimental) Build the project with no CMake cache."
    echo "  -r,--refresh-ddl  (Experimental) Build the project with refreshed DDL."
    echo "  -v,--verbose      Show lots of information while building the project."
    echo "  -h,--help         Display this help text."
    echo " "
    echo "Note: The --refresh-ddl flag explicitly drops the database to ensure that the"
    echo "      DDL file is regenerated from scratch.  Please use accordingly."
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    NO_CACHE=0
    REFRESH_DDL=0
    FORCE_BUILD=0
    LINT_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -nc|--no-cache)
            NO_CACHE=1
            shift
        ;;
        -l|--lint)
            LINT_MODE=1
            shift
        ;;
        -r|--refresh-ddl)
            REFRESH_DDL=1
            shift
        ;;
        -f|--force)
            FORCE_BUILD=1
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

# Generate the Makefile
generate_makefile() {
    if [ $VERBOSE_MODE -ne 0 ]; then
        echo "Generating the makefile..."
        cmake -B "$BUILD_DIRECTORY"
        DID_FAIL=$?
    else
        cmake -B "$BUILD_DIRECTORY" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >"$TEMP_FILE" 2>&1
        DID_FAIL=$?
    fi
    if [ $DID_FAIL -ne 0 ]; then
        if [ $VERBOSE_MODE -eq 0 ]; then
            cat "$TEMP_FILE"
        fi
        complete_process 1 "Generation of the makefile failed."
    fi
}

# Build the actual executable file.
invoke_makefile() {
    if [ $VERBOSE_MODE -ne 0 ]; then
        echo "Building the executable..."
    fi
    if ! make -C "$BUILD_DIRECTORY" --silent; then
        complete_process 1 "Build of the executable failed."
    fi
}

# Ensure that the build directory is in the required state to start the build.
prepare_build_directory() {
    # Force a complete rebuild of everything.
    if [ $FORCE_BUILD -eq 1 ]; then
        if [ $VERBOSE_MODE -ne 0 ]; then
            echo "Force option selected. Forcing rebuild of all build artifacts."
        fi
        rm -rf "$BUILD_DIRECTORY"
    fi

    # If the build directory does not already exist, then create it.
    if [ ! -d "$BUILD_DIRECTORY" ]; then
        # Note that at this point, even if the flag has not been set, we
        #   treat the build as a forced build to ensure everything is set
        #   properly.
        FORCE_BUILD=1

        if ! mkdir "$BUILD_DIRECTORY"; then
            complete_process 1 "Creation of the build directory failed."
        fi
    fi
}

# Handle the no CMAKE cache flag and the refresh DDL flags.
#
# Note that currently, both of these flags are experimental.
handle_optional_flags() {
    # If the flag is set to not use the cached CMake data, then remove it first.
    if [ $NO_CACHE -eq 1 ]; then
        if [ $VERBOSE_MODE -ne 0 ]; then
            echo "No-cache option selected.  Removing CMake cache file."
        fi
        rm "$BUILD_DIRECTORY/CMakeCache.txt"
    fi

    # If the flag is set to refresh the DDL data, then do so.
    if [ $REFRESH_DDL -eq 1 ]; then
        if [ $VERBOSE_MODE -ne 0 ]; then
            echo "Refresh-ddl option selected.  Removing database '$DATABASE_NAME'."
        fi
        echo "drop database $DATABASE_NAME" > "$TEMP_FILE"
        echo "exit" >> "$TEMP_FILE"
        gaiac -i < "$TEMP_FILE" > /dev/null
        rm -rf "$BUILD_DIRECTORY/gaia_generated/edc"
    fi
}



# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# shellcheck disable=SC1091 source=./properties.sh
source "$SCRIPTPATH/properties.sh"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.build.tmp

# Set up any local script variables.

parse_command_line "$@"

# Clean entrance into the script.
start_process

prepare_build_directory

handle_optional_flags

generate_makefile

invoke_makefile

if [ $LINT_MODE -ne 0 ]; then
    if ! ./lint.sh  > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        complete_process 1 "Linting of the project after a successful build failed."
    fi
fi

# If we get here, we have a clean exit from the script.
complete_process 0
