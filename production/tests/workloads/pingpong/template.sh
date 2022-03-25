#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Applying templates for the $PROJECT_NAME project..."
        echo ""
    fi

    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1

    if ! cd "$SCRIPTPATH" >"$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot change to coverage directory before proceeding."
    fi
}

# Simple function to stop the process, including any cleanup.
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo ""
        echo "Applying templates for the $PROJECT_NAME project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo ""
            echo "Applying templates for the $PROJECT_NAME project succeeded."
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
    echo "  -r,--remove                 Remove the templated files from the project."
    echo "  -v,--verbose                Show lots of information while executing the project."
    echo "  -h,--help                   Display this help text."
    echo ""
    show_usage_commands
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    REMOVE_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -r|--remove)
            REMOVE_MODE=1
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

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
TEMPLATE_PATH="no-template-path"
# shellcheck disable=SC1091,SC1090
source "$SCRIPTPATH/properties.sh"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.template.tmp
DID_PUSHD=0

# Parse any command line values.
parse_command_line "$@"

# Make sure we have a template specified.
if [ -z $TEMPLATE_PATH ] || [ $TEMPLATE_PATH == "no-template-path" ]; then
    complete_process 1 "Variable TEMPLATE_PATH was not specified by project's 'properties.sh' file."
fi

# Clean entrance into the script.
start_process

# Verify the requirements for templating are in place.
if [ -z "$TEMPLATE_PATH" ] ; then
    complete_process 1 "Template directory environment variable TEMPLATE_PATH is not set."
fi
if [ ! -d "$TEMPLATE_PATH" ] ; then
    complete_process 1 "Template directory '$(realpath "$TEMPLATE_PATH")' does not exist."
fi
TEMPLATE_FILE=$TEMPLATE_PATH/template.txt
if [ ! -f "$TEMPLATE_FILE" ] ; then
    complete_process 1 "Template source file '$(realpath "$TEMPLATE_FILE")' does not exist."
fi

# The template file contains a single file name per line.  That file name is relative
# to the template file itself, and specifies the source file name of the templated
# file.  The local file name is relative to this script file.
# shellcheck disable=SC2016
IFS=$'\r\n' GLOBIGNORE='*' command eval  'TEMPLATE_FILE_NAMES=($(cat $TEMPLATE_FILE))'

# Before doing the templating, make sure each file in the template file exists.
for NEXT_TEMPLATE_FILE_NAME in "${TEMPLATE_FILE_NAMES[@]}"; do
    SOURCE_PATH=$TEMPLATE_PATH/$NEXT_TEMPLATE_FILE_NAME
    if [ ! -f "$SOURCE_PATH" ] ; then
        complete_process 1 "Template file '$(realpath "$SOURCE_PATH")' does not exist."
    fi
done

# Do the templating to bring the required files in, or remove them.
for NEXT_TEMPLATE_FILE_NAME in "${TEMPLATE_FILE_NAMES[@]}"; do
    SOURCE_PATH=$TEMPLATE_PATH/$NEXT_TEMPLATE_FILE_NAME
    DESTINATION_PATH=$SCRIPTPATH/$NEXT_TEMPLATE_FILE_NAME
    FILE_DESTINATION_DIR=$(dirname "$DESTINATION_PATH")

    if [[ $REMOVE_MODE -eq 1 ]] ; then
        if [ ! -f "$DESTINATION_PATH" ] ; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Template file '$DESTINATION_PATH' already removed."
            fi
        else
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Removing template file '$DESTINATION_PATH'."
            fi
            if ! rm "$DESTINATION_PATH" > "$TEMP_FILE" 2>&1 ; then
                cat "$TEMP_FILE"
                complete_process 1 "Template file '$(realpath "$DESTINATION_PATH")' cannot be removed."
            fi
        fi

        if [ -d "$FILE_DESTINATION_DIR" ]; then
            if [ ! "$(ls -A "$FILE_DESTINATION_DIR")" ]; then
                if [ "$VERBOSE_MODE" -ne 0 ]; then
                    echo "Removing template directory '$FILE_DESTINATION_DIR'."
                fi
                if ! rmdir "$FILE_DESTINATION_DIR" ; then
                    cat "$TEMP_FILE"
                    complete_process 1 "Template directory '$(realpath "$FILE_DESTINATION_DIR")' cannot be removed."
                fi
            fi
        fi
    else
        if [ ! -d "$FILE_DESTINATION_DIR" ] ; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Creating templated directory '$FILE_DESTINATION_DIR'."
            fi
            if ! mkdir -p "$FILE_DESTINATION_DIR" > "$TEMP_FILE" 2>&1 ; then
                cat "$TEMP_FILE"
                complete_process 1 "Template directory '$(realpath "$FILE_DESTINATION_DIR")' cannot be created."
            fi
        fi

        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Installing template file '$DESTINATION_PATH'"
            echo "  from '$SOURCE_PATH'."
        fi
        if ! cp "$SOURCE_PATH" "$DESTINATION_PATH" > "$TEMP_FILE" 2>&1 ; then
            cat "$TEMP_FILE"
            complete_process 1 "Template file '$(realpath "$DESTINATION_PATH")' cannot be copied from '$(realpath "$SOURCE_PATH")'."
        fi
    fi
done

# If we get here, we have a clean exit from the script.
complete_process 0
