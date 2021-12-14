#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Linting the $PROJECT_NAME project..."
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
        echo "Linting of the $PROJECT_NAME project failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Linting of the $PROJECT_NAME project succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
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
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
    echo ""
    show_usage_commands
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
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
        echo "Saving current directory prior to linting."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Lint script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Check to ensure that pipenv is installed and ready to go.
verify_correct_pipenv_installed() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Verifying installation of the Python PIPENV application."
    fi
    if ! pip3 list pipenv | grep pipenv > "$TEMP_FILE" 2>&1; then
        if ! pip3 install pipenv > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Cannot install pipenv for verification of Python scripts."
        fi
    fi

    # Due to the Pipfile, when synced, the following libraries should be loaded into pipenv:
    #
    # black==21.7b0, flake8==3.9.2, pylint==2.9.5
    if ! pipenv lock > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Cannot sync pipenv for verification of Python scripts."
    fi
    # pipenv install black==21.7b0
    # pipenv install flake8==3.9.2
    # pipenv install pylint==2.9.5
}

# Lint the python scripts.
lint_python_scipts() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Applying formatting to the Python parts of the $PROJECT_NAME project."
    fi
    if ! pipenv run black "$PYTHON_SOURCE_DIRECTORY" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Cannot reformat Python scripts according to PEP8 standard."
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Analyzing the Python parts of the $PROJECT_NAME project using Flake8."
    fi
    # shellcheck disable=SC2086
    if ! pipenv run flake8 --ignore=E203,E501,W503 $PYTHON_SOURCE_DIRECTORY/*.py > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Python Flake8 scanner revealed issues with scripts."
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Analyzing the Python parts of the $PROJECT_NAME project using PyLint."
    fi
    # shellcheck disable=SC2086
    if ! pipenv run pylint --disable=R0801 -sn $PYTHON_SOURCE_DIRECTORY/*.py > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Python PyLint scanner revealed issues with scripts."
    fi
}

# Check to ensure that shellcheck is installed and ready to go.
verify_correct_shellcheck_installed() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Verifying installation of the Bash Shellcheck Analyzer."
    fi
    local SHELLCHECK_VERSION_ACTUAL_FILE=/tmp/lint.clang.tidy.out
    local SHELLCHECK_VERSION_ACTUAL_FILEx=/tmp/lint.clang.tidy.outx
    local SHELLCHECK_VERSION_REQUIRED_FILE=/tmp/lint.clang.tidy.orig
    local SHELLCHECK_REQUIRED_VERSION_LINE_1="ShellCheck - shell script analysis tool"
    local SHELLCHECK_REQUIRED_VERSION_LINE_2="version: 0.7.0"

    if ! command -v shellcheck &>/dev/null; then
        complete_process 1 "Please install shellcheck 0.7.0."
    fi

    if ! shellcheck --version > "$SHELLCHECK_VERSION_ACTUAL_FILEx" 2>&1 ; then
        complete_process 1 "Cannot execute 'shellcheck' to determine if it has the correct version."
    fi
    head -n 2 $SHELLCHECK_VERSION_ACTUAL_FILEx > $SHELLCHECK_VERSION_ACTUAL_FILE

    echo "$SHELLCHECK_REQUIRED_VERSION_LINE_1" > $SHELLCHECK_VERSION_REQUIRED_FILE
    echo "$SHELLCHECK_REQUIRED_VERSION_LINE_2" >> $SHELLCHECK_VERSION_REQUIRED_FILE
    if ! diff -w "$SHELLCHECK_VERSION_REQUIRED_FILE" "$SHELLCHECK_VERSION_ACTUAL_FILE" &>/dev/null ; then
        actual_line=$(tail -n 1 "$SHELLCHECK_VERSION_ACTUAL_FILE")
        expected_line=$(head -n 2 "$SHELLCHECK_VERSION_REQUIRED_FILE")
        complete_process 1 "shellcheck version is '$actual_line' instead of '$expected_line'."
    fi
}

# Lint the shell scripts.
lint_shell_scripts() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Analyzing the Bash parts of the $PROJECT_NAME project."
    fi
    if ! shellcheck -x ./*.sh > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Linting of shellscript by 'shellcheck' failed."
    fi
    if [ -d ./utils ] ; then
        if ! shellcheck -x ./utils/*.sh > "$TEMP_FILE" 2>&1; then
            cat "$TEMP_FILE"
            complete_process 1 "Linting of shellscript by 'shellcheck' failed."
        fi
    fi
}

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.lint.tmp

# Set up any local script variables.
PYTHON_SOURCE_DIRECTORY=$SCRIPTPATH

# Parse any command line values.
parse_command_line "$@"

# Verify that we have the right tools installed.
verify_correct_pipenv_installed
verify_correct_shellcheck_installed

# Clean entrance into the script.
start_process

save_current_directory

# Lint the various parts of the project.
#lint_python_scipts
lint_shell_scripts

complete_process 0
