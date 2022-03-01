#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to verify that any child Workflow file is properly composed from the main Workflow file.
"""

import argparse
import glob
import os
import sys

# The following is disable is required.  When used as part of pre-commit, the package is
# explicitly installed by the pre-commit configuration.
#
# pylint: disable=import-error
import yaml

# pylint: enable=import-error

__DEFAULT_FILE_ENCODING = "utf-8"
__FUNGIBLE_JOB_PROPERTIES = {"needs", "if", "timeout-minutes"}
__NON_FUNGIBLE_PROPERTIES = {"runs-on", "env", "steps", "strategy"}


def __process_command_line():
    """
    Process the arguments on the command line.
    """

    parser = argparse.ArgumentParser(description="Fill in.")
    parser.add_argument(
        "--base-workflow",
        dest="base_workflow",
        action="store",
        required=True,
        help="Base workflow to compare other workflows against.",
    )
    parser.add_argument(
        "--ignore",
        dest="ignore_path",
        action="store",
        help="Path of a workflow to ignore.",
    )
    parser.add_argument("filenames", nargs="+", help="Filenames to fix")
    return parser.parse_args()


def __calculate_github_workflows_directory():

    current_script_directory = os.path.dirname(os.path.realpath(__file__))
    base_project_directory = os.path.join(current_script_directory, "..", "..", "..")
    base_project_directory = os.path.abspath(base_project_directory)
    workflows_directory = os.path.join(base_project_directory, ".github", "workflows")
    return workflows_directory


def __load_workflow_file(file_name):

    full_file_name = os.path.join(__calculate_github_workflows_directory(), file_name)
    try:
        with open(full_file_name, "r", encoding=__DEFAULT_FILE_ENCODING) as file:
            return yaml.safe_load(file), full_file_name
    except IOError as this_exception:
        print(f"Unable to open workflow file '{full_file_name}': {this_exception}")
    except yaml.YAMLError as this_exception:
        print(
            f"Unable to open workflow file '{full_file_name}' as a YAML file: {this_exception}"
        )
    return None, full_file_name


def __selectively_compare_jobs(job_name, copy_job_dictionary, main_workflow_dictionary):

    for job_property_name in main_workflow_dictionary:
        if (
            job_property_name not in __FUNGIBLE_JOB_PROPERTIES
            and job_property_name not in __NON_FUNGIBLE_PROPERTIES
        ):
            print(
                f"Main workflow job '{job_name}' contains a property '{job_property_name}' "
                + "that has not been accounted for."
            )
            return False

    did_error = False
    for job_property_name in copy_job_dictionary:
        if job_property_name in __FUNGIBLE_JOB_PROPERTIES:
            continue

        if job_property_name not in __NON_FUNGIBLE_PROPERTIES:
            print(
                f"Job '{job_name}' contains a property '{job_property_name}' that "
                + "has not been accounted for."
            )
            did_error = True
        elif job_property_name not in main_workflow_dictionary:
            print(
                f"Job '{job_name}' contains a property '{job_property_name}' that "
                + "is not in the main workflow's job."
            )
            did_error = True
        elif (
            copy_job_dictionary[job_property_name]
            != main_workflow_dictionary[job_property_name]
        ):
            print(
                f"Job '{job_name}' contains a property '{job_property_name}' that "
                + "does not contain the exact same contents as the main workflow's job."
            )
            did_error = True

    return not did_error


def __compare_workflow_files(main_workflow_dictionary, copy_workflow_dictionary):

    if main_workflow_dictionary.keys() != copy_workflow_dictionary.keys():
        print("High-level workflow keys must be the same in both workflows.")
        print(f"Main workflow keys: {main_workflow_dictionary.keys()}")
        print(f"Scan workflow keys: {copy_workflow_dictionary.keys()}")
        return False

    if main_workflow_dictionary["env"] != copy_workflow_dictionary["env"]:
        print("Workflow level 'env' configuration must be the same in both workflows.")
        print(f"Main workflow keys: {main_workflow_dictionary['env']}")
        print(f"Scan workflow keys: {copy_workflow_dictionary['env']}")
        return False

    did_any_jobs_have_errors = False
    for job_name in copy_workflow_dictionary["jobs"]:
        if job_name not in main_workflow_dictionary["jobs"]:
            print(
                f"Workflow job '{job_name}' is present in the child workflow, "
                + "but not the main workflow."
            )
            did_any_jobs_have_errors = True
        elif not __selectively_compare_jobs(
            job_name,
            copy_workflow_dictionary["jobs"][job_name],
            main_workflow_dictionary["jobs"][job_name],
        ):
            did_any_jobs_have_errors = True

    return not did_any_jobs_have_errors


def __scan_child_workflow(main_workflow_dictionary, next_filename):

    did_scan_succeed = False
    copy_workflow_dictionary, _ = __load_workflow_file(next_filename)
    did_scan_succeed = bool(copy_workflow_dictionary)
    if did_scan_succeed:
        did_scan_succeed = __compare_workflow_files(
            main_workflow_dictionary, copy_workflow_dictionary
        )

    if not did_scan_succeed:
        print(f"Scan of file '{next_filename}' failed.")

    return did_scan_succeed


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    main_workflow_dictionary, main_workflow_file_name = __load_workflow_file(
        args.base_workflow
    )
    if not main_workflow_dictionary:
        return 1

    scan_error_count = 0
    for next_filename in args.filenames:

        if "*" in next_filename:
            ignore_path = os.path.join(
                __calculate_github_workflows_directory(), args.ignore_path
            )
            glob_path = os.path.join(
                __calculate_github_workflows_directory(), next_filename
            )
            for next_glob_filename in glob.glob(glob_path):
                if next_glob_filename == main_workflow_file_name:
                    continue

                if ignore_path == next_glob_filename:
                    print(f"Ignoring: {next_glob_filename}")
                    continue

                did_scan_succeed = __scan_child_workflow(
                    main_workflow_dictionary, next_glob_filename
                )
                if not did_scan_succeed:
                    scan_error_count += 1
        else:
            did_scan_succeed = __scan_child_workflow(
                main_workflow_dictionary, next_filename
            )
            if not did_scan_succeed:
                scan_error_count += 1

    return bool(scan_error_count)


sys.exit(process_script_action())
