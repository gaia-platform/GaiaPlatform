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
    echo "  -v,--verbose        Show lots of information while executing the project."
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

# Check to ensure that the right version of clang-format is installed.
verify_correct_clang_format_installed() {
    local CLANG_FORMAT_VERSION_ACTUAL_FILE=/tmp/lint.clang.format.out
    local CLANG_FORMAT_VERSION_REQUIRED_FILE=/tmp/lint.clang.format.orig
    local CLANG_FORMAT_REQUIRED_VERSION="clang-format version 10.0.0-4ubuntu1"

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Verifying installation of the Clang-Format application."
    fi

    if ! command -v clang-format &>/dev/null; then
        complete_process 1 "Please install clang-format 10.0."
    fi

    if ! clang-format --version > "$CLANG_FORMAT_VERSION_ACTUAL_FILE" 2>&1 ; then
        complete_process 1 "Cannot execute 'clang-format' to determine if it has the correct version."
    fi
    echo "$CLANG_FORMAT_REQUIRED_VERSION" > "$CLANG_FORMAT_VERSION_REQUIRED_FILE"
    if ! diff -w "$CLANG_FORMAT_VERSION_REQUIRED_FILE" "$CLANG_FORMAT_VERSION_ACTUAL_FILE" > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        actual_line=$(head -n 1 "$CLANG_FORMAT_VERSION_ACTUAL_FILE")
        expected_line=$(head -n 1 "$CLANG_FORMAT_VERSION_REQUIRED_FILE")
        complete_process 1 "clang-format version is '$actual_line' instead of '$expected_line'."
    fi
}

# Check to ensure that the right version of clang-tidy is installed.
verify_correct_clang_tidy_installed() {
    local CLANG_TIDY_VERSION_ACTUAL_FILE=/tmp/lint.clang.tidy.out
    local CLANG_TIDY_VERSION_ACTUAL_FILEx=/tmp/lint.clang.tidy.outx
    local CLANG_TIDY_VERSION_REQUIRED_FILE=/tmp/lint.clang.tidy.orig
    local CLANG_TIDY_REQUIRED_VERSION_LINE_1="LLVM (http://llvm.org/):"
    local CLANG_TIDY_REQUIRED_VERSION_LINE_2="LLVM version 10.0.0"

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Verifying installation of the Clang-Tidy application."
    fi

    if ! command -v clang-tidy &>/dev/null; then
        complete_process 1 "Please install clang-tidy 10.0."
    fi

    if ! clang-tidy --version > "$CLANG_TIDY_VERSION_ACTUAL_FILEx" 2>&1 ; then
        complete_process 1 "Cannot execute 'clang-tidy' to determine if it has the correct version."
    fi
    head -n 2 $CLANG_TIDY_VERSION_ACTUAL_FILEx > $CLANG_TIDY_VERSION_ACTUAL_FILE

    echo "$CLANG_TIDY_REQUIRED_VERSION_LINE_1" > $CLANG_TIDY_VERSION_REQUIRED_FILE
    echo "$CLANG_TIDY_REQUIRED_VERSION_LINE_2" >> $CLANG_TIDY_VERSION_REQUIRED_FILE
    if ! diff -w "$CLANG_TIDY_VERSION_REQUIRED_FILE" "$CLANG_TIDY_VERSION_ACTUAL_FILE" &>/dev/null ; then
        actual_line=$(tail -n 1 "$CLANG_TIDY_VERSION_ACTUAL_FILE")
        expected_line=$(head -n 2 "$CLANG_TIDY_VERSION_REQUIRED_FILE")
        complete_process 1 "clang-tidy version is '$actual_line' instead of '$expected_line'."
    fi
}

# Build the project.  Without this, the C++ lint information may be out of date.
build_project() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Building the $PROJECT_NAME project."
    fi
    if ! ./build.sh -v > "$TEMP_FILE" 2>&1 ; then
        cat "$TEMP_FILE"
        echo "Build script cannot build the project."
        complete_process 1
    fi
}

# Lint the C++ code.
lint_c_plus_plus_code() {

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Building the $PROJECT_NAME project to ensure the C++ components to scan are current."
    fi
    build_project

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Applying formatting to the C++ parts of the $PROJECT_NAME project."
    fi

    if ! clang-format -i "./mink.cpp" --style=file > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Formatting of 'mink.cpp' failed."
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Analyzing the C++ parts of the $PROJECT_NAME project."
    fi
    if ! clang-tidy --warnings-as-errors=* -p "build" -extra-arg="-std=c++17" "./mink.cpp" -- -I/opt/gaia/include "-I$SCRIPTPATH/build/gaia_generated/edc/mink" > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "File 'mink.cpp' contains some lint errors."
    fi
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
}

# Lint the python scripts.
lint_python_scipts() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Applying formatting to the Python parts of the $PROJECT_NAME project."
    fi
    if ! pipenv run black python > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Cannot reformat Python scripts according to PEP8 standard."
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Analyzing the Python parts of the $PROJECT_NAME project using Flake8."
    fi
    if ! pipenv run flake8 --ignore=E203,E501,W503 python > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Python Flake8 scanner revealed issues with scripts."
    fi

    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Analyzing the Python parts of the $PROJECT_NAME project using PyLint."
    fi
    if ! pipenv run pylint -sn python > "$TEMP_FILE" 2>&1; then
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
    if ! shellcheck ./*.sh > "$TEMP_FILE" 2>&1; then
        cat "$TEMP_FILE"
        complete_process 1 "Linting of shellscript by 'shellcheck' failed."
    fi
}


# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=/tmp/$PROJECT_NAME.lint.tmp

# Set up any local script variables.

# Parse any command line values.
parse_command_line "$@"

# Verify that we have the right tools installed.
# verify_correct_clang_format_installed
# verify_correct_clang_tidy_installed
# verify_correct_pipenv_installed
verify_correct_shellcheck_installed

# Clean entrance into the script.
start_process

save_current_directory

# Lint the various parts of the project.
# lint_c_plus_plus_code
# lint_python_scipts
lint_shell_scripts

complete_process 0
