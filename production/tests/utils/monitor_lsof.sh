#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

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

find_gaia_db_server_pid
if [ -z "$gaia_db_server_pid" ] ; then
    echo "ProcessID for 'gaia_db_server' cannot be found."
    echo "Please start the Gaia server before starting this script."
    exit 2
fi

db_server_pid=$gaia_db_server_pid
sudo /usr/bin/bash -c "rm -f /tmp/lsof >/dev/null 2>&1"
sudo /usr/bin/bash -c "rm -f /tmp/lsof.old >/dev/null 2>&1"

PROCESS_IS_ACTIVE=1
while [ $PROCESS_IS_ACTIVE -eq 1 ]
    do
        sudo mv /tmp/lsof /tmp/lsof.old
        if ! sudo /usr/bin/bash -c "lsof -p $db_server_pid > /tmp/lsof 2>&1" ; then
            echo "------"
            echo "Command 'lsof' cannot return a list of desciptors.  Terminating..."
            PROCESS_IS_ACTIVE=0
            continue
        fi

        sudo /usr/bin/bash -c "diff /tmp/lsof /tmp/lsof.old > /tmp/lsof.diff 2>&1"
        lines_in_diff=$(wc -l /tmp/lsof.diff | cut -d' ' -f1)
        if [ "$lines_in_diff" -ne 0 ] ; then
            cat /tmp/lsof.diff
        fi

        sleep 0.1
        if ! find_gaia_db_server_pid ; then
            echo "------"
            echo "ProcessID for 'gaia_db_server' is no longer present.  Terminating..."
            PROCESS_IS_ACTIVE=0
        fi
        if [ ! "$gaia_db_server_pid" == "$db_server_pid" ] ; then
            echo "------"
            echo "ProcessID for 'gaia_db_server' has changed.  Terminating..."
            PROCESS_IS_ACTIVE=0
        fi
    done
