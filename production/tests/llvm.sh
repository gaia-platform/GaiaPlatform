#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Executing the LLVM Level 2 tests..."
    fi
}

# Simple function to stop the process, including any cleanup
complete_process() {
    local SCRIPT_RETURN_CODE=$1
    local COMPLETE_REASON=$2

    if [ -n "$COMPLETE_REASON" ] ; then
        echo "$COMPLETE_REASON"
    fi

    kill_database_if_was_started

    if [ "$SCRIPT_RETURN_CODE" -ne 0 ]; then
        echo "Executing the LLVM Level 2 tests failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Executing the LLVM Level 2 tests succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi
    if [ -f "$SCRIPTPATH/database.log" ]; then
        rm "$SCRIPTPATH/database.log"
    fi

    # Restore the current directory.
    if [ "$DID_PUSHD" -eq 1 ]; then
        popd > /dev/null 2>&1 || exit
    fi

    exit "$SCRIPT_RETURN_CODE"
}

# Show how this script can be used.
show_usage() {
    local SCRIPT_NAME=$0

    echo "Usage: $(basename "$SCRIPT_NAME") [flags]"
    echo "Flags:"
    echo "  -d,--database       Enable the creation of the database within the script."
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
    echo ""
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    START_DATABASE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -d|--database)
            START_DATABASE=1
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

# Save the current directory when starting the script, so we can go back to that
# directory at the end of the script.
save_current_directory() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Saving current directory prior to execution."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

kill_database_if_was_started() {

    if [ -n "$DB_SERVER_PID" ] ; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Killing started database server."
        fi
        kill "$DB_SERVER_PID"
        DB_SERVER_PID=
    fi
}

is_database_started() {
    SERVER_PID=$(pgrep "gaia_db_server")
    if [[ -n $SERVER_PID ]] ; then
        echo "1"
    else
        echo "0"
    fi
}

start_database_if_required() {
    if [ "$START_DATABASE" -ne 0 ]; then
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Creating a new database service instance for the test suite."
        fi

        SERVER_PID=$(pgrep "gaia_db_server")
        if [[ -n $SERVER_PID ]] ; then
            if [ "$VERBOSE_MODE" -ne 0 ]; then
                echo "Stopping currently executing database instance."
            fi

            if ! kill "$SERVER_PID" ; then
                complete_process 1 "Test suite failed to stop existing database service instance. Stop that instance manually before trying again."
            fi
            sleep 2
            SERVER_PID=$(pgrep "gaia_db_server")
            if [[ -n $SERVER_PID ]] ; then
                complete_process 1 "Test suite verification of stopped database service instance failed. Stop that instance manually before trying again."
            fi
        fi

        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Starting new database instance as a background process."
        fi
        PERSISTENCE_FLAG="--persistence disabled"
        # shellcheck disable=SC2086
        gaia_db_server $PERSISTENCE_FLAG > "$SCRIPTPATH/database.log" 2>&1 &
        # shellcheck disable=SC2181
        if [ $? -ne 0 ] ; then
            cat "$SCRIPTPATH/database.log"
            complete_process 1 "Cannot start the requested database service instance."
        fi

        # Five seconds is way more than enough for 95% of the cases, but may be needed in
        # some cases with persistence.
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Pausing for 5 seconds before verifying that the new database service instance is responsive."
        fi
        DB_SERVER_PID=$(pgrep "gaia_db_server")
        sleep 5
        DB_SERVER_PID=
        SERVER_PID=$(pgrep "gaia_db_server")
        if [[ -z $SERVER_PID ]] ; then
            echo "Active processes"
            echo "----"
            ps
            echo "----"
            cat "$SCRIPTPATH/database.log"
            complete_process 1 "Newly started database service instance was not responsive.  Cannot continue the test suite without an active database instance."
        fi
        DB_SERVER_PID=$SERVER_PID
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Created a new database service instance for the test suite."
        fi

        # Make sure that we register a trap handler to be able to kill the database
        # if the suite stops.
        trap kill_database_if_was_started EXIT
    fi
}

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=$(mktemp /tmp/llvm.XXXXXXXXX)

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory

cd /build/production || exit

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Building required components."
fi
if ! make count > "$TEMP_FILE" 2>&1 ; then
    cat "$TEMP_FILE"
    complete_process 1 "Cannot create the required 'count' component."
fi
if ! make not > "$TEMP_FILE" 2>&1 ; then
    cat "$TEMP_FILE"
    complete_process 1 "Cannot create the required 'not' component."
fi

cd /source/production/tools/tests/gaiat || exit

if ! update-alternatives --install "/usr/local/bin/FileCheck" "FileCheck" "/usr/lib/llvm-13/bin/FileCheck" 10 ; then
    complete_process 1 "Required 'FileCheck' component not setup."
fi

start_database_if_required

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Priming the database with 'barn_storage.dll'."
fi

if [[ $(is_database_started) -eq 0 ]] ; then
    echo "Gaia database required for GaiaC and not started."
    complete_process 1 "Either start it externally or use the -d argument to create a database for the duration of the script."
fi
if ! /build/production/catalog/gaiac/gaiac /source/third_party/production/TranslationEngineLLVM/clang/test/Parser/barn_storage.ddl ; then
    complete_process 1 "Required priming of the database with GaiaC not completed."
fi

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Executing the LLVM Tests."
fi
if ! /build/production/llvm/bin/llvm-lit --xunit-xml-output /build/production/output/llvm.xml . ; then
    complete_process 1 "Execution of the LLVM tests failed."
fi

kill_database_if_was_started

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Execution of the LLVM tests succeeded."
fi
complete_process 0
