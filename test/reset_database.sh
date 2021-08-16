#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Resetting the database..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Resetting the database failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Resetting the database succeeded."
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
    echo "  -v,--verbose                Show lots of information while executing the project."
    echo "  -h,--help                   Display this help text."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    RESET_DATABASE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -d|--database)
            RESET_DATABASE=1
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

# Figure out the state of the gaia service.
get_service_state() {
    systemctl_output=$(systemctl status "$service_name" | grep -E "Active\: ([a-z]+)")
    regex="Active\: ([a-z]+)"
    if [[ $systemctl_output =~ $regex ]] ; then
        service_state="${BASH_REMATCH[1]}"
    else
        echo "Output for systemctl not as expected."
        complete_process 1
    fi
}

# Wait until the service gets into the specified state.
wait_for_service_state() {
    local state_to_wait_for=$1

    number_of_seconds=5
    while [ $number_of_seconds -gt 0 ]; do
        get_service_state
        if [[ "$service_state" == "$state_to_wait_for" ]] ; then
            break
        fi

        number_of_seconds=$(("$number_of_seconds"-1))
        sleep 1
    done

    if ! [[ "$service_state" == "$state_to_wait_for" ]] ; then
        echo "Service $service_name did not transition to the $state_to_wait_for state."
        complete_process 1
    fi
}

# Stop the database service.
stop_database_service() {
    get_service_state
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Service $service_name is currently $service_state."
    fi

    if [ "$service_state" != "inactive" ] && [ "$service_state" != "failed" ]; then
        echo "Setting service $service_name to 'inactive'."
        if ! sudo systemctl stop gaia ; then
            echo "Service $service_name cannot be stopped."
            complete_process 1
        fi

        wait_for_service_state "inactive"

        get_service_state
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Service $service_name is currently $service_state."
        fi
    fi
}

# Start the database service.
start_database_service() {
    get_service_state
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Service $service_name is currently $service_state."
    fi

    if ! [ "$service_state" == "active" ] ; then
        echo "Setting service $service_name to 'active'."
        if ! sudo systemctl start gaia ; then
            echo "Service $service_name cannot be started."
            complete_process 1
        fi

        wait_for_service_state "active"

        get_service_state
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Service $service_name is currently $service_state."
        fi
    fi
}

# Remove the datastore files from their default location.
remove_data_store() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Removing gaia data store files from default location."
    fi
    if ! sudo rm -rf /var/lib/gaia/db ; then
        echo "Gaia data store files cannot be removed."
        complete_process 1
    fi
}

# If an install file was specified, make sure it is present and looks like a debian install file.
verify_install_file_if_present() {
    INSTALL_FILE=${PARAMS[0]}
    if [ -n "$INSTALL_FILE" ] ; then

        # Make sure it exists.
        if [ ! -f "$INSTALL_FILE" ]; then
            echo "Install file '$(realpath "$INSTALL_FILE")' does not exist."
            complete_process 1
        fi

        if [[ ! "$INSTALL_FILE" == *.deb ]] ; then
            echo "Install file '$(realpath "$INSTALL_FILE")' does not end with the required suffix '.deb'."
            complete_process 1
        fi

        echo "Installation of new packages requires resetting of the database.  Option selected."
        RESET_DATABASE=1
    fi
}

# Remove the old package and install the new package.
install_new_package() {
    if ! sudo apt --assume-yes remove gaia ; then
        echo "Removal of old Gaia package did not complete.  Gaia may be in an undefined state."
        complete_process 1
    fi

    if ! sudo apt --assume-yes install "$INSTALL_FILE" ; then
        echo "Installation of new Gaia package did not complete.  Gaia may be in an undefined state."
        complete_process 1
    fi
}

# Set up any global script variables.
# shellcheck disable=SC2164
#SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=/tmp/reset.db.tmp

# Set up any local script variables.
service_name=gaia


# Parse any command line values.
parse_command_line "$@"

verify_install_file_if_present


# Clean entrance into the script.
start_process

stop_database_service

if [ $RESET_DATABASE -ne 0 ] ; then
    remove_data_store
fi

if [ -n "$INSTALL_FILE" ] ; then
    install_new_package
fi

start_database_service

# If we get here, we have a clean exit from the script.
complete_process 0
