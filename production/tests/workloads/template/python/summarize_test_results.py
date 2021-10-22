#! /usr/bin/python3

"""
Script to calculate the test_summary.json file for a suite of tests that were run.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import json
import sys
import os

TEST_RESULTS_DIRECTORY = "test-results/"
TEST_RESULTS_FILE = "test-summary.json"

ITERATIONS_TITLE = "iterations"
SAMPLE_TIME_TITLE = "sample-ms"
CONFIGURATION_FILE_TITLE = "configuration-file"
RETURN_CODE_TITLE = "return-code"
TOTAL_DURATION_TITLE = "duration-sec"


# pylint: disable=broad-except
def __load_simple_result_files(base_dir):
    """
    Load the simple, one value, result files.

    The return_code.json is from the test.sh and stored by the suite.sh file.
    The duration.json is from the run.sh and stored by the test.sh file.,
    """

    json_path = os.path.join(base_dir, "return_code.json")
    with open(json_path) as input_file:
        data = json.load(input_file)
        return_code_data = data[RETURN_CODE_TITLE]

    duration_data = 0.0
    json_path = os.path.join(base_dir, "duration.json")
    try:
        with open(json_path) as input_file:
            data = json.load(input_file)
            duration_data = data["duration"]
    except Exception as my_exception:
        duration_data = (
            "Duration data could not be loaded from "
            + f"original '{json_path}' file ({my_exception})."
        )
    return return_code_data, duration_data


# pylint: enable=broad-except


def __load_test_result_files(suite_test_directory, test_configuration_file):
    """
    Load sets of individual results from their various sources.

    The various sources are used because it is easier for each component
    to emit its own data without having to worry about the integrity of
    a single file.
    """

    base_dir = (
        suite_test_directory
        if os.path.isabs(suite_test_directory)
        else os.path.join(TEST_RESULTS_DIRECTORY, suite_test_directory)
    )

    return_code_data, duration_data = __load_simple_result_files(base_dir)

    iterations_data = 1
    sample_data = 1

    new_results = {}
    new_results[ITERATIONS_TITLE] = iterations_data
    new_results[SAMPLE_TIME_TITLE] = sample_data
    new_results[CONFIGURATION_FILE_TITLE] = test_configuration_file
    new_results[RETURN_CODE_TITLE] = return_code_data
    new_results[TOTAL_DURATION_TITLE] = duration_data

    return new_results


def __dump_results_dictionary(output_directory, full_test_results):
    """
    Dump the full_test_results dictionary as a JSON file.
    """
    output_path = os.path.join(output_directory, TEST_RESULTS_FILE)
    print(f"Creating test results file: {str(os.path.abspath(output_path))}")
    with open(f"{output_path}", "w") as write_file:
        json.dump(full_test_results, write_file, indent=4)


def __process_script_action():
    """
    Process the posting of the message.
    """

    if len(sys.argv) < 2:
        print("Test results directory must be specified as the second parameter.")
        sys.exit(1)
    test_results_directory = sys.argv[1]
    if not os.path.exists(test_results_directory) or os.path.isfile(
        test_results_directory
    ):
        print(
            f"Test results directory '{test_results_directory}' must exist and be a directory."
        )
        sys.exit(1)

    if len(sys.argv) < 3:
        print("Test configuration file must be specified as the third parameter.")
        sys.exit(1)
    test_configuration_file = sys.argv[2]

    results_dictionary = __load_test_result_files(
        test_results_directory, test_configuration_file
    )
    __dump_results_dictionary(test_results_directory, results_dictionary)


sys.exit(__process_script_action())
