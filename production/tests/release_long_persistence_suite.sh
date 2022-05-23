#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
#
# Note that these three variables, SUITE_MODE, TEST_WORKLOADS, and USE_PERSISTENT_DATABASE
# are used within the sourced script.  To keep shellcheck happy, we disable them.

# shellcheck disable=SC2034
SUITE_MODE="long-persistence-perf-suites"
# shellcheck disable=SC2034
TEST_WORKLOADS=("largest-scale")
# shellcheck disable=SC2034
USE_PERSISTENT_DATABASE=1

# Invoke the actual execution of the suites.
# shellcheck disable=SC1091
source "$SCRIPTPATH/execute_suites.sh"
execute_suites "$@"
