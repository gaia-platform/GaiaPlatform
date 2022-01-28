#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Installing a new SDK build..."
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
        echo "Installation of the new SDK build failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Installation of the new SDK build succeeded."
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

    echo "Usage: $(basename "$SCRIPT_NAME") [flags] new_debian_install_file"
    echo "Flags:"
    echo "  -s,--show                   Show the path to the last DEB package that was installed."
    echo "  -v,--verbose                Display detailed information during execution."
    echo "  -h,--help                   Display this help text."
    echo "Parameters:"
    echo "  new_debian_install_file     Filename of a debian build file to install while the"
    echo "                              database is shutdown."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    SHOW_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -s|--show)
            SHOW_MODE=1
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

# Make sure that we do not have any instances running, not even as
# a normal execution.
find_gaia_db_server_pid() {
    local gaia_output=

    gaia_db_server_pid=
    gaia_output=$(pgrep -f "gaia_db_server")
    if [ -z "$gaia_output" ] ; then
        return 1
    fi
    # shellcheck disable=SC2086
    gaia_db_server_pid=$(echo $gaia_output | cut -d' ' -f2)
}

# Make sure that the install file is present and looks like a debian install file.
verify_install_file() {
    INSTALL_FILE=${PARAMS[0]}

    if [ -z "$INSTALL_FILE" ] ; then
        complete_process 1 "Install file '$(realpath "$INSTALL_FILE")' does not exist."
    fi

    # Make sure it exists.
    if [ ! -f "$INSTALL_FILE" ]; then
        complete_process 1 "Install file '$(realpath "$INSTALL_FILE")' does not exist."
    fi

    if [[ ! "$INSTALL_FILE" == *.deb ]] ; then
        complete_process 1 "Install file '$(realpath "$INSTALL_FILE")' does not end with the required suffix '.deb'."
    fi
}

# Remove the old package and install the new package.
install_new_package() {
    if ! sudo apt --assume-yes remove gaia ; then
        complete_process 1 "Removal of old Gaia package did not complete.  Gaia may be in an undefined state."
    fi

    if ! sudo apt --assume-yes install "$INSTALL_FILE" ; then
        complete_process 1 "Installation of new Gaia package did not complete.  Gaia may be in an undefined state."
    fi
}

# Set up any global script variables.
# shellcheck disable=SC2164
#SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=/tmp/intall_sdk.db.tmp

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process

if [ $SHOW_MODE -ne 0 ] ; then
    echo "Installed DEB pacakage as follows:"
    cat /opt/gaia/installed.txt
    complete_process 0
fi

verify_install_file

# Stop any installed `gaia` service that is running.
if ! ./reset_database.sh --stop --database ; then
    complete_process 1 "Database service stop did not complete."
fi

# Make sure the executable isn't being run manually.
find_gaia_db_server_pid
if [ -n "$gaia_db_server_pid" ] ; then
    echo "Database executable is being run manually."
    echo "Please stop that executable before attempting again."
    complete_process 1
fi

# Install the new package.  Note that the last step of the install
# is to start the new DB server as a service.  As such, we don't
# need this script to balance out the stop service with a start service.
install_new_package

sudo bash -c "echo 'Installed: $INSTALL_FILE' > /opt/gaia/installed.txt"

# If we get here, we have a clean exit from the script.
complete_process 0
