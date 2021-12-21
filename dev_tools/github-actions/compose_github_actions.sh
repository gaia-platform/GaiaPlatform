#! /usr/bin/bash

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Simple function to start the process off.
start_process() {
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Creating the GitHub Actions files for the project..."
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
        echo "Creating the GitHub Actions files failed."
    else
        if [ "$VERBOSE_MODE" -ne 0 ]; then
            echo "Creating the GitHub Actions files succeeded."
        fi
    fi

    if [ -f "$TEMP_FILE" ]; then
        rm "$TEMP_FILE"
    fi
    if [ -f "$OTHER_TEMP_FILE" ]; then
        rm "$OTHER_TEMP_FILE"
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

# Set up any global script variables.
# shellcheck disable=SC2164
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# Set up any project based local script variables.
TEMP_FILE=$(mktemp /tmp/compose_github_actions.XXXXXXXXX)
OTHER_TEMP_FILE=$(mktemp /tmp/compose_github_actions.XXXXXXXXX)

# Set up any local script variables.
DESTINATION_WORKFLOW_FILE=../../.github/workflows/main.yml

# Parse any command line values.
parse_command_line "$@"

# Clean entrance into the script.
start_process
save_current_directory
cd "$SCRIPTPATH" || exit

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating the Jobs header for the Workflow file."
fi
if ! ./build_job_header.py > "$TEMP_FILE" ; then
    cat "$TEMP_FILE"
    complete_process 1 "Error creating the Jobs header for the Workflow file."
fi
cat "$TEMP_FILE" > "$DESTINATION_WORKFLOW_FILE"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating the Lint job for the Workflow file."
fi
if ! ./build_job_section.py \
    --job-name "Lint" \
    --action=lint \
    --directory ../../production  \
    --option ubuntu:20.04 --option CI_GitHub --option Lint > "$TEMP_FILE" ; then
    cat "$TEMP_FILE"
    complete_process 1 "Error creating the Lint job for the Workflow file."
fi
cat "$TEMP_FILE" >> "$DESTINATION_WORKFLOW_FILE"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating the Core job for the Workflow file."
fi
if ! ./build_job_section.py \
    --job-name "Core" \
    --requires "Lint" \
    --directory ../../production \
    --option ubuntu:20.04 --option CI_GitHub > "$TEMP_FILE" ; then
    cat "$TEMP_FILE"
    complete_process 1 "Error creating the Core job for the Workflow file."
fi
cat "$TEMP_FILE" >> "$DESTINATION_WORKFLOW_FILE"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating the SDK job for the Workflow file."
fi
if ! ./build_job_section.py \
    --job-name "SDK" \
    --requires "Core" \
    --directory ../../production \
    --option GaiaRelease --option ubuntu:20.04 --option CI_GitHub > "$TEMP_FILE" ; then
    cat "$TEMP_FILE"
    complete_process 1 "Error creating the SDK job for the Workflow file."
fi
cat "$TEMP_FILE" >> "$DESTINATION_WORKFLOW_FILE"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating the Debug SDK job for the Workflow file."
fi
if ! ./build_job_section.py \
    --job-name "Debug_SDK" \
    --requires "Core" \
    --directory ../../production \
    --option Debug --option ubuntu:20.04 --option CI_GitHub > "$TEMP_FILE" ; then
    cat "$TEMP_FILE"
    complete_process 1 "Error creating the Debug SDK job for the Workflow file."
fi
cat "$TEMP_FILE" >> "$DESTINATION_WORKFLOW_FILE"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Creating the LLVM Tests job for the Workflow file."
fi
if ! ./build_job_section.py \
    --job-name "LLVM_Tests" \
    --requires "Core" \
    --directory ../../production \
    --option GaiaLLVMTests --option ubuntu:20.04 --option CI_GitHub > "$TEMP_FILE" ; then
    cat "$TEMP_FILE"
    complete_process 1 "Error creating the LLVM Tests job for the Workflow file."
fi
cat "$TEMP_FILE" >> "$DESTINATION_WORKFLOW_FILE"

if [ "$VERBOSE_MODE" -ne 0 ]; then
    echo "Generating list of actions to take with a newly installed build artifact."
fi
if ! ./munge_gdev_files.py --directory ../../production  --section installs --job-name x > "$OTHER_TEMP_FILE" ; then
    cat "$OTHER_TEMP_FILE"
    complete_process 1 "Error generating the list of actions to take with the build artifact."
fi

while IFS="" read -r ACTION || [ -n "$ACTION" ]
do
    if [ "$VERBOSE_MODE" -ne 0 ]; then
        echo "Generating job '$ACTION'."
    fi
    if ! ./build_job_section.py \
        --job-name "$ACTION" \
        --requires "SDK" \
        --directory ../../production \
        --option ubuntu:20.04 \
        --action "install:$ACTION" > "$TEMP_FILE" ; then
        cat "$TEMP_FILE"
        complete_process 1 "Error generating job '$ACTION'."
    fi
    cat "$TEMP_FILE" >> "$DESTINATION_WORKFLOW_FILE"
done < "$OTHER_TEMP_FILE"
complete_process 0
