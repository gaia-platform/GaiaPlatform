#!/usr/bin/env bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
#
# Note that these four variables, SUITE_MODE, TEST_WORKLOADS, USE_PERSISTENT_DATABASE,
# and USE_MEMORY_SAMPLING are used within the sourced script.  To keep shellcheck happy,
# we disable them.

# shellcheck disable=SC2034
SUITE_MODE="long-persistence-perf-suites"
# shellcheck disable=SC2034
TEST_WORKLOADS=("largest-scale")
# shellcheck disable=SC2034
USE_PERSISTENT_DATABASE=0
# shellcheck disable=SC2034
USE_MEMORY_SAMPLING=1

# Invoke the actual execution of the suites.
source "$SCRIPTPATH/execute_suites.sh"
execute_suites "$@"
