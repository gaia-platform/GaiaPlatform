#!/usr/bin/env bash

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
#
# Note that these four variables, SUITE_MODE, TEST_WORKLOADS, USE_PERSISTENT_DATABASE,
# and USE_MEMORY_SAMPLING are used within the sourced script.  To keep shellcheck happy,
# we disable them.

# shellcheck disable=SC2034
SUITE_MODE="perf-suites"
# shellcheck disable=SC2034
TEST_WORKLOADS=("mink-perf" "small-scale" "pingpong-perf" "marcopolo-perf")
# shellcheck disable=SC2034
USE_PERSISTENT_DATABASE=0
# shellcheck disable=SC2034
USE_MEMORY_SAMPLING=0

# Invoke the actual execution of the suites.
# Shellcheck has issues following dynamic paths to validate files. There is an open github
# issue (2176) in the koalaman/shellcheck repo to use the active path to check the soure files.
# Until then disable this check.
# shellcheck disable=SC1091
source "$SCRIPTPATH/execute_suites.sh"
execute_suites "$@"
