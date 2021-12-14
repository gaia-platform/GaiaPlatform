#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# TODO: check for clang-format version.

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Verifying C++ file format for the $PROJECT_NAME project..."
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
        echo "Verifying C++ file format failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Verifying C++ file format succeeded."
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
    echo "  -f,--fix            Attempt to fix any non-fatal errors."
    echo "  -v,--verbose        Display detailed information during execution."
    echo "  -h,--help           Display this help text."
    echo ""
    show_usage_commands
    exit 1
}

# Parse the command line.
parse_command_line() {
    VERBOSE_MODE=0
    FIX_MODE=0
    PARAMS=()
    while (( "$#" )); do
    case "$1" in
        -v|--verbose)
            VERBOSE_MODE=1
            shift
        ;;
        -f|--fix)
            FIX_MODE=1
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
        echo "Saving current directory prior to scanning."
    fi
    if ! pushd . >"$TEMP_FILE" 2>&1;  then
        cat "$TEMP_FILE"
        complete_process 1 "Script cannot save the current directory before proceeding."
    fi
    DID_PUSHD=1
}

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=$(mktemp /tmp/pre-commit.XXXXXXXXX)

# Set up any local script variables.

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory

cd "$SCRIPTPATH" || exit
cd ../.. || exit

# check if clang-format is installed.
if ! command -v clang-format &>/dev/null; then
    complete_process 1 "Please install clang-format 10.0 and try again."
fi

#### 1. check for forbidden patterns in header files ####

# Forbidden header strings
forbidden_patterns=(
    "#include <iostream>"
    "using namespace"
)

did_any_fail=0
for file in $(find production | grep -E "^(production).*(\.hpp|\.inc)$"); do
    for forbidden_pattern in "${forbidden_patterns[@]}"; do
    if grep -Fq "$forbidden_pattern" "$file"; then
        echo "File '$file' contains forbidden pattern: '$forbidden_pattern'";
        did_any_fail=1
    fi
    done
done

if [ $did_any_fail -ne 0 ]; then
    echo ""
    echo "One or more header files contain forbidden files.  Failing format check."
    complete_process 1 "Remove the specified pattern from each file and try again."
fi

#### 2. apply formatting to all files ####

# Files excluded from clang-format (interpreted as regex against the full path of the file)
excluded_files=(
    "gaia_catalog\.h"
    "catalog_generated\.h"
    "gaia_catalog\.cpp"
    "json\.hpp"
)

# Runs clang-format only on files that:
# 1. Are in the index (git add)
# 2. Are in the production/ directory
# 3. End with .hpp/.cpp/etc..
did_any_fail=0
for file in $(find production | grep -E "^(production|demo).*(\.hpp|\.cpp|\.inc)$|^\.clang-format$"); do
    format_file=true

    for exclude_pattern in "${excluded_files[@]}"; do
        if echo "$file" | grep -qE "$exclude_pattern"; then
            format_file=false
            break
        fi
    done

    if [ "$format_file" = "true" ]; then
        clang-format -i -output-replacements-xml "$file" | grep "<replacement " >/dev/null
        if [ $? -ne 1 ]; then

            if [ $FIX_MODE -ne 0 ]; then
                echo "Clang-format mismatch for file $file"
                echo "  Formatting file and staging file to git."
                clang-format -i "$file"
                git add "$file"
            else
                echo ""
                echo "Clang-format mismatch for file $file"
                echo "---"
                cp "$file" test.cpp
                clang-format -i test.cpp
                diff --report-identical-files "$file" test.cpp
                rm test.cpp
                echo "---RAW"
                clang-format -i -output-replacements-xml "$file"
                echo "---"
                did_any_fail=1
            fi
        fi
    fi
done

if [ $did_any_fail -ne 0 ]; then
    echo ""
    echo "One or more cpp files failed clang-format verification.  Failing format check."
    complete_process 1 "Execute the pre-commit script locally to fix the reported errors, and try again."
fi

complete_process 0
