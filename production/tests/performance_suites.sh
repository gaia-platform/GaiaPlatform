#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
#
# Note that these three variables, SUITE_MODE, TEST_WORKLOADS, and USE_PERSISTENT_DATABASE
# are used within the sourced script.  To keep shellcheck happy, we disable them.

# shellcheck disable=SC2034
SUITE_MODE="perf-suites"
# shellcheck disable=SC2034
TEST_WORKLOADS=("mink-perf" "small-scale" "palletbox-perf"  "pingpong-perf" "marcopolo-perf")
# shellcheck disable=SC2034
USE_PERSISTENT_DATABASE=0

# Invoke the actual execution of the suites.
# shellcheck source=execute_suites.sh
source "$SCRIPTPATH/execute_suites.sh"
execute_suites "$@"
