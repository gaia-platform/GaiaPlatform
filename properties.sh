# shellcheck shell=bash

# ----------------------------------------------------
# Variables that are specific to this project.
# ----------------------------------------------------

# Name of the project.  Safe to include in filenames, etc.
export PROJECT_NAME="incubator"

# Name of the project's executable within the build directory.
export EXECUTABLE_NAME="incubator"

# Name of the project's database.
export DATABASE_NAME="incubator"

# Various parts that are built to generate the executable.
# Used for allowing an "autobuild" option on run.sh.
export RULESET_PATHS=("incubator.ruleset")
export DDL_PATHS=("incubator.ddl")
export SOURCE_PATHS=("incubator.cpp")

# ----------------------------------------------------
# Variables that are general to this group of scripts.
# ----------------------------------------------------

# Relative directory where the build results are stored.
export BUILD_DIRECTORY="build"

# Relative directory where the logs are stored.
export LOG_DIRECTORY="logs"

# Relative directory where the test results are stored.
export TEST_RESULTS_DIRECTORY="test-results"
