#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Script to calculate the test_summary.json file for a suite of tests that were run.
"""

import json
import sys
import os

__DEFAULT_FILE_ENCODING = "utf-8"

DECIMALS_PLACES_IN_NANOSECONDS = 9
MICROSEC_PER_SEC = 1000000

TEST_RESULTS_DIRECTORY = "test-results/"
TEST_RESULTS_FILE = "test-summary.json"

ITERATIONS_TITLE = "iterations"
CONFIGURATION_FILE_TITLE = "configuration-file"
RETURN_CODE_TITLE = "return-code"
TOTAL_DURATION_TITLE = "duration-sec"
TEST_DURATION_TITLE = "test-duration-sec"
ITERATION_DURATION_TITLE = "iteration-duration-sec"
PAUSE_DURATION_TITLE = "stop-pause-sec"
WAIT_DURATION_TITLE = "wait-pause-sec"
PRINT_DURATION_TITLE = "print-duration-sec"

AVG_WAIT_DURATION_TITLE = "average-wait-microsec"
AVG_PRINT_DURATION_TITLE = "average-print-microsec"

T_PAUSE_TITLE = "t-requested-pause-microseconds"
T_OVER_PERCENT_TITLE = "t-over-percent"
T_PAUSE_SECONDS_TITLE = "t-pause-sec"
T_REQUESTED_SECONDS_TITLE = "t-requested-sec"
T_EXCESS_NANOSECONDS_TITLE = "t-excess-microsec"
T_PER_EXCESS_NANOSECONDS_TITLE = "t-individual-excess-microsec"

START_TRANSACTION_DURATION_TITLE = "start-transaction-sec"
INSIDE_TRANSACTION_DURATION_TITLE = "inside-transaction-sec"
END_TRANSACTION_DURATION_TITLE = "end-transaction-sec"
UPDATE_ROW_DURATION_TITLE = "update-row-sec"

AVG_START_TRANSACTION_DURATION_TITLE = "average-start-transaction-microsec"
AVG_INSIDE_TRANSACTION_DURATION_TITLE = "average-inside-transaction-microsec"
AVG_END_TRANSACTION_DURATION_TITLE = "average-end-transaction-microsec"
AVG_UPDATE_ROW_DURATION_TITLE = "average-update-row-microsec"

MEASURED_DURATION_TITLE = "measured-duration-sec"
PER_MEASURED_DURATION_TITLE = "iteration-measured-duration-sec"


# pylint: disable=broad-except
def __load_simple_result_files(base_dir):
    """
    Load the simple, one value, result files.

    The return_code.json is from the test.sh and stored by the suite.sh file.
    The duration.json is from the run.sh and stored by the test.sh file.,
    """

    json_path = os.path.join(base_dir, "return_code.json")
    with open(json_path, encoding=__DEFAULT_FILE_ENCODING) as input_file:
        data = json.load(input_file)
        return_code_data = data[RETURN_CODE_TITLE]

    duration_data = 0.0
    json_path = os.path.join(base_dir, "duration.json")
    try:
        with open(json_path, encoding=__DEFAULT_FILE_ENCODING) as input_file:
            data = json.load(input_file)
            duration_data = data["duration"]
    except Exception as my_exception:
        duration_data = (
            "Duration data could not be loaded from "
            + f"original '{json_path}' file ({my_exception})."
        )
    return return_code_data, duration_data


# pylint: disable=too-many-locals
def __load_output_timing_files(base_dir):
    """
    Load the 'output.delay' file generated from the main executable.
    """

    (
        stop_pause_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        measured_section_data,
        t_pause_data,
        t_requested_data,
        t_config_data,
        start_transaction_data,
        inside_transaction_data,
        end_transaction_data,
        update_row_data,
    ) = (0.0, 0, 0.0, 0.0, None, 0.0, None, None, 0.0, 0.0, 0.0, 0.0)

    json_path = os.path.join(base_dir, "output.delay")
    if os.path.exists(json_path):
        with open(json_path, encoding=__DEFAULT_FILE_ENCODING) as input_file:
            data = json.load(input_file)
            stop_pause_data = data["stop_pause_in_sec"]
            iterations_data = data["iterations"]
            total_wait_data = data["total_wait_in_sec"]
            total_print_data = data["total_print_in_sec"]
            t_pause_data = data["t_pause_in_sec"]
            t_requested_data = data["t_requested_in_sec"]

            start_transaction_data = data["start_transaction_in_sec"]
            inside_transaction_data = data["inside_transaction_in_sec"]
            end_transaction_data = data["end_transaction_in_sec"]
            update_row_data = data["update_row_in_sec"]

            measured_section_data = None
            if "measured_in_sec" in data:
                measured_section_data = data["measured_in_sec"]
            t_config_data = None
            if "requested_t_pause_in_microseconds" in data:
                t_config_data = data["requested_t_pause_in_microseconds"]

    return (
        stop_pause_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        measured_section_data,
        t_pause_data,
        t_requested_data,
        t_config_data,
        start_transaction_data,
        inside_transaction_data,
        end_transaction_data,
        update_row_data,
    )


def __handle_t_data(
    new_results, t_pause_data, t_requested_data, t_config_data, iterations_data
):
    if t_config_data:
        new_results[T_PAUSE_TITLE] = t_config_data
    if t_pause_data > 0.00001:
        new_results[T_PAUSE_SECONDS_TITLE] = t_pause_data
        new_results[T_REQUESTED_SECONDS_TITLE] = t_requested_data
        new_results[T_EXCESS_NANOSECONDS_TITLE] = int(
            (t_pause_data - t_requested_data) * 1000000.0
        )
        new_results[T_OVER_PERCENT_TITLE] = round(
            ((t_pause_data - t_requested_data) / t_pause_data) * 100.0, 3
        )
        if iterations_data > 0:
            new_results[T_PER_EXCESS_NANOSECONDS_TITLE] = int(
                ((t_pause_data - t_requested_data) / iterations_data) * 1000000.0
            )


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
    (
        stop_pause_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        measured_section_data,
        t_pause_data,
        t_requested_data,
        t_config_data,
        start_transaction_data,
        inside_transaction_data,
        end_transaction_data,
        update_row_data,
    ) = __load_output_timing_files(base_dir)

    new_results = {}
    new_results[ITERATIONS_TITLE] = iterations_data
    new_results[CONFIGURATION_FILE_TITLE] = test_configuration_file
    new_results[RETURN_CODE_TITLE] = return_code_data
    new_results[TOTAL_DURATION_TITLE] = duration_data
    new_results[PAUSE_DURATION_TITLE] = stop_pause_data
    new_results[WAIT_DURATION_TITLE] = total_wait_data
    new_results[PRINT_DURATION_TITLE] = total_print_data
    __handle_t_data(
        new_results, t_pause_data, t_requested_data, t_config_data, iterations_data
    )
    new_results[START_TRANSACTION_DURATION_TITLE] = start_transaction_data
    new_results[INSIDE_TRANSACTION_DURATION_TITLE] = inside_transaction_data
    new_results[END_TRANSACTION_DURATION_TITLE] = end_transaction_data
    new_results[UPDATE_ROW_DURATION_TITLE] = update_row_data

    if new_results[ITERATIONS_TITLE] > 0:
        new_results[AVG_START_TRANSACTION_DURATION_TITLE] = round(
            (start_transaction_data / float(new_results[ITERATIONS_TITLE]))
            * MICROSEC_PER_SEC,
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        new_results[AVG_INSIDE_TRANSACTION_DURATION_TITLE] = round(
            inside_transaction_data
            / float(new_results[ITERATIONS_TITLE])
            * MICROSEC_PER_SEC,
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        new_results[AVG_END_TRANSACTION_DURATION_TITLE] = round(
            end_transaction_data
            / float(new_results[ITERATIONS_TITLE])
            * MICROSEC_PER_SEC,
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        new_results[AVG_UPDATE_ROW_DURATION_TITLE] = round(
            update_row_data / float(new_results[ITERATIONS_TITLE]) * MICROSEC_PER_SEC,
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        new_results[AVG_WAIT_DURATION_TITLE] = round(
            (total_wait_data / float(new_results[ITERATIONS_TITLE])) * MICROSEC_PER_SEC,
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        new_results[AVG_PRINT_DURATION_TITLE] = round(
            (total_print_data / float(new_results[ITERATIONS_TITLE]))
            * MICROSEC_PER_SEC,
            DECIMALS_PLACES_IN_NANOSECONDS,
        )

    if not isinstance(new_results[TOTAL_DURATION_TITLE], str):
        new_results[TEST_DURATION_TITLE] = round(
            new_results[TOTAL_DURATION_TITLE]
            - new_results[PAUSE_DURATION_TITLE]
            - new_results[WAIT_DURATION_TITLE]
            - new_results[PRINT_DURATION_TITLE],
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        if new_results[ITERATIONS_TITLE] > 0:
            new_results[ITERATION_DURATION_TITLE] = round(
                new_results[TEST_DURATION_TITLE] / float(new_results[ITERATIONS_TITLE]),
                DECIMALS_PLACES_IN_NANOSECONDS,
            )
    if measured_section_data:
        new_results[MEASURED_DURATION_TITLE] = measured_section_data
        if new_results[ITERATIONS_TITLE] > 0:
            new_results[PER_MEASURED_DURATION_TITLE] = round(
                measured_section_data / float(new_results[ITERATIONS_TITLE]),
                DECIMALS_PLACES_IN_NANOSECONDS,
            )

    return new_results


# pylint: enable=too-many-locals


def __dump_results_dictionary(output_directory, full_test_results):
    """
    Dump the full_test_results dictionary as a JSON file.
    """
    output_path = os.path.join(output_directory, TEST_RESULTS_FILE)
    print(f"Creating test results file: {str(os.path.abspath(output_path))}")
    with open(f"{output_path}", "w", encoding=__DEFAULT_FILE_ENCODING) as write_file:
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
