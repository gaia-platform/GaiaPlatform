#! /usr/bin/python3

"""
Script to calculate the summary.json file for a suite of tests that were run.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import json
import sys
import os
import re
import configparser

SUITE_DIRECTORY = "suite-results/"

THREAD_LOAD_INDEX = 5
SCHEDULED_INDEX = 7
INVOKED_INDEX = 8
PENDING_INDEX = 9
ABANDONED_INDEX = 10
RETRY_INDEX = 11
EXCEPTION_INDEX = 12
AVERAGE_LATENCY_INDEX = 13
MAXIMUM_LATENCY_INDEX = 15
AVERAGE_EXEC_INDEX = 17
MAXIMUM_EXEC_INDEX = 19

THREAD_LOAD_TITLE = "thread-load-percent"

CONFIGURATION_TITLE = "configuration"
ITERATIONS_TITLE = "iterations"
RETURN_CODE_TITLE = "return-code"
TEST_DURATION_TITLE = "test-duration-sec"
ITERATION_DURATION_TITLE = "iteration-duration-sec"
TOTAL_DURATION_TITLE = "duration-sec"
PAUSE_DURATION_TITLE = "stop-pause-sec"
WAIT_DURATION_TITLE = "wait-pause-sec"
PRINT_DURATION_TITLE = "print-duration-sec"
TEST_RUNS_TITLE = "test_runs"

RULES_ENGINE_TITLE = "rules-engine-stats"
RULES_ENGINE_SLICES_TITLE = "slices"
RULES_ENGINE_TOTALS_TITLE = "totals"
RULES_ENGINE_CALCULATIONS_TITLE = "calculations"

SCHEDULED_TITLE = "scheduled"
INVOKED_TITLE = "invoked"
PENDING_TITLE = "pending"
ABANDONED_TITLE = "abandoned"
RETRY_TITLE = "retry"
EXCEPTION_TITLE = "exception"

AVERAGE_LATENCY_TITLE = "avg-latency-ms"
MAXIMUM_LATENCY_TITLE = "max-latency-ms"
AVERAGE_EXEC_TITLE = "avg-exec-ms"
MAXIMUM_EXEC_TITLE = "max-exec-ms"


def calculate_total_counts(stats_slice, log_line_columns, totals, title, index):
    """
    Given another sliace, update the total number of elements for a given title.
    """

    stats_slice[title] = int(log_line_columns[index])
    previous_value = 0
    if title in totals:
        previous_value = totals[title]
    totals[title] = previous_value + stats_slice[title]


def calculate_maximum_values(stats_slice, log_line_columns, calculations, title, index):
    """
    Given another slice, update the maximum values if needed for any title.
    """

    stats_slice[title] = float(log_line_columns[index])
    previous_maximum = 0.0
    if title in calculations:
        previous_maximum = calculations[title]
    calculations[title] = max(previous_maximum, stats_slice[title])


def calculate_average_values(stats_slice, log_line_columns, calculations, title, index):
    """
    Given another slice, calculate any totals we need for average values.

    Note: These values are weighted by the number of scheduled rules that
          were processed.  Before reporting, that number must be divided by the
          total number of scheduled rules to get a proper average.
    """

    stats_slice[title] = float(log_line_columns[index])
    previous_value = 0
    if title in calculations:
        previous_value = calculations[title]
    calculations[title] = previous_value + (
        stats_slice[title] * stats_slice[SCHEDULED_TITLE]
    )


def process_rules_engine_stats(base_dir):
    """
    Load up the `gaia_stats.log` file for a given test and convert it into
    a JSON blob that can be attached to the summary.
    """

    log_path = os.path.join(base_dir, "gaia_stats.log")
    with open(log_path) as input_file:
        log_file_lines = input_file.readlines()

    stats_slices = []
    totals = {}
    calculations = {}
    for next_log_line in log_file_lines:
        next_log_line = next_log_line.strip()
        while "  " in next_log_line:
            next_log_line = next_log_line.replace("  ", " ")
        log_line_columns = next_log_line.split(" ")
        if log_line_columns[3].startswith("--"):
            continue
        if not log_line_columns[3].startswith("[thread"):
            continue
        assert len(log_line_columns) == MAXIMUM_EXEC_INDEX + 2, str(log_line_columns)

        stats_slice = {}
        stats_slice[THREAD_LOAD_TITLE] = float(log_line_columns[THREAD_LOAD_INDEX])
        calculate_total_counts(
            stats_slice, log_line_columns, totals, SCHEDULED_TITLE, SCHEDULED_INDEX
        )
        calculate_total_counts(
            stats_slice, log_line_columns, totals, INVOKED_TITLE, INVOKED_INDEX
        )
        calculate_total_counts(
            stats_slice, log_line_columns, totals, PENDING_TITLE, PENDING_INDEX
        )
        calculate_total_counts(
            stats_slice, log_line_columns, totals, ABANDONED_TITLE, ABANDONED_INDEX
        )
        calculate_total_counts(
            stats_slice, log_line_columns, totals, RETRY_TITLE, RETRY_INDEX
        )
        calculate_total_counts(
            stats_slice, log_line_columns, totals, EXCEPTION_TITLE, EXCEPTION_INDEX
        )
        calculate_average_values(
            stats_slice,
            log_line_columns,
            calculations,
            AVERAGE_LATENCY_TITLE,
            AVERAGE_LATENCY_INDEX,
        )
        calculate_maximum_values(
            stats_slice,
            log_line_columns,
            calculations,
            MAXIMUM_LATENCY_TITLE,
            MAXIMUM_LATENCY_INDEX,
        )
        calculate_average_values(
            stats_slice,
            log_line_columns,
            calculations,
            AVERAGE_EXEC_TITLE,
            AVERAGE_EXEC_INDEX,
        )
        calculate_maximum_values(
            stats_slice,
            log_line_columns,
            calculations,
            MAXIMUM_EXEC_TITLE,
            MAXIMUM_EXEC_INDEX,
        )
        stats_slices.append(stats_slice)

    calculations[AVERAGE_LATENCY_TITLE] = calculations[AVERAGE_LATENCY_TITLE] / float(
        totals[SCHEDULED_TITLE]
    )
    calculations[AVERAGE_EXEC_TITLE] = calculations[AVERAGE_EXEC_TITLE] / float(
        totals[SCHEDULED_TITLE]
    )

    rules_engine_stats = {}
    rules_engine_stats[RULES_ENGINE_SLICES_TITLE] = stats_slices
    rules_engine_stats[RULES_ENGINE_TOTALS_TITLE] = totals
    rules_engine_stats[RULES_ENGINE_CALCULATIONS_TITLE] = calculations
    return rules_engine_stats


def process_configuration_file(base_dir):
    """
    Load up the generated "*.conf" file for the test and translate the
    "Rules" portion into a JSON blob.
    """

    json_path = os.path.join(base_dir, "incubator.conf")
    config = configparser.ConfigParser()
    config.read(json_path)
    assert "Rules" in config
    rules_configuration = {}
    for key in config["Rules"]:
        this_value = config["Rules"][key]
        if this_value.lower() == "true" or this_value.lower() == "false":
            rules_configuration[key] = this_value.lower() == "true"
        elif re.search("^([0-9]+)$", this_value):
            rules_configuration[key] = int(this_value)
        else:
            rules_configuration[key] = this_value
    return rules_configuration


def load_simple_result_files(base_dir):
    """
    Load the simple, one value, result files.

    The return_code.json is from the test.sh and stored by the suite.sh file.
    The duration.json is from the run.sh and stored by the test.sh file.,
    """

    json_path = os.path.join(base_dir, "return_code.json")
    with open(json_path) as input_file:
        data = json.load(input_file)
        return_code_data = data[RETURN_CODE_TITLE]

    json_path = os.path.join(base_dir, "duration.json")
    with open(json_path) as input_file:
        data = json.load(input_file)
        duration_data = data["duration"]
    return return_code_data, duration_data


def load_output_timing_files(base_dir):
    """
    Load the 'output.delay' file generated from the main executable.
    """

    json_path = os.path.join(base_dir, "output.delay")
    with open(json_path) as input_file:
        data = json.load(input_file)
        stop_pause_data = data["stop_pause_in_sec"]
        iterations_data = data["iterations"]
        total_wait_data = data["total_wait_in_sec"]
        total_print_data = data["total_print_in_sec"]

    return stop_pause_data, iterations_data, total_wait_data, total_print_data


def load_test_result_files(suite_test):
    """
    Load sets of individual results from their various sources.

    The various sources are used because it is easier for each component
    to emit its own data without having to worry about the integrity of
    a single file.
    """

    base_dir = os.path.join(SUITE_DIRECTORY, suite_test)

    return_code_data, duration_data = load_simple_result_files(base_dir)
    (
        stop_pause_data,
        iterations_data,
        total_wait_data,
        total_print_data,
    ) = load_output_timing_files(base_dir)

    stats_data = process_rules_engine_stats(base_dir)

    configuration_data = process_configuration_file(base_dir)

    return (
        return_code_data,
        duration_data,
        stop_pause_data,
        stats_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        configuration_data,
    )


def load_results_for_test(suite_test):
    """
    Load all the results for tests and place them in the main dictionary.
    """

    (
        return_code_data,
        duration_data,
        stop_pause_data,
        stats_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        configuration_data,
    ) = load_test_result_files(suite_test)

    new_results = {}
    new_results[CONFIGURATION_TITLE] = configuration_data
    new_results[ITERATIONS_TITLE] = iterations_data
    new_results[RETURN_CODE_TITLE] = return_code_data
    new_results[TOTAL_DURATION_TITLE] = duration_data
    new_results[PAUSE_DURATION_TITLE] = stop_pause_data
    new_results[WAIT_DURATION_TITLE] = total_wait_data
    new_results[PRINT_DURATION_TITLE] = total_print_data

    new_results[TEST_DURATION_TITLE] = (
        new_results[TOTAL_DURATION_TITLE]
        - new_results[PAUSE_DURATION_TITLE]
        - new_results[WAIT_DURATION_TITLE]
        - new_results[PRINT_DURATION_TITLE]
    )
    new_results[ITERATION_DURATION_TITLE] = new_results[TEST_DURATION_TITLE] / float(
        new_results[ITERATIONS_TITLE]
    )
    new_results[RULES_ENGINE_TITLE] = stats_data
    return new_results


def summarize_repeated_tests(suite_test_name, max_test):
    """
    Create a summary dictionary for any repeated tests.
    """

    main_dictionary = {}

    main_dictionary[ITERATIONS_TITLE] = []
    main_dictionary[RETURN_CODE_TITLE] = []
    main_dictionary[TEST_DURATION_TITLE] = []
    main_dictionary[ITERATION_DURATION_TITLE] = []

    totals = {}
    totals[SCHEDULED_TITLE] = []
    totals[INVOKED_TITLE] = []
    totals[PENDING_TITLE] = []
    totals[ABANDONED_TITLE] = []
    totals[RETRY_TITLE] = []
    totals[EXCEPTION_TITLE] = []

    calculations = {}
    calculations[AVERAGE_LATENCY_TITLE] = []
    calculations[MAXIMUM_LATENCY_TITLE] = []
    calculations[AVERAGE_EXEC_TITLE] = []
    calculations[MAXIMUM_EXEC_TITLE] = []

    rules_engine_stats = {}
    rules_engine_stats[RULES_ENGINE_TOTALS_TITLE] = totals
    rules_engine_stats[RULES_ENGINE_CALCULATIONS_TITLE] = calculations

    main_dictionary[RULES_ENGINE_TITLE] = rules_engine_stats

    test_runs = {}
    main_dictionary[TEST_RUNS_TITLE] = test_runs
    for test_repeat in range(1, max_test + 1):
        recorded_name = suite_test_name + "_" + str(test_repeat)
        new_results = load_results_for_test(recorded_name)

        main_dictionary[ITERATIONS_TITLE].append(new_results[ITERATIONS_TITLE])
        main_dictionary[RETURN_CODE_TITLE].append(new_results[RETURN_CODE_TITLE])
        main_dictionary[TEST_DURATION_TITLE].append(new_results[TEST_DURATION_TITLE])
        main_dictionary[ITERATION_DURATION_TITLE].append(
            new_results[ITERATION_DURATION_TITLE]
        )

        test_totals = new_results[RULES_ENGINE_TITLE][RULES_ENGINE_TOTALS_TITLE]
        totals[SCHEDULED_TITLE].append(test_totals[SCHEDULED_TITLE])
        totals[INVOKED_TITLE].append(test_totals[INVOKED_TITLE])
        totals[PENDING_TITLE].append(test_totals[PENDING_TITLE])
        totals[ABANDONED_TITLE].append(test_totals[ABANDONED_TITLE])
        totals[RETRY_TITLE].append(test_totals[RETRY_TITLE])
        totals[EXCEPTION_TITLE].append(test_totals[EXCEPTION_TITLE])

        test_calculations = new_results[RULES_ENGINE_TITLE][
            RULES_ENGINE_CALCULATIONS_TITLE
        ]
        calculations[AVERAGE_LATENCY_TITLE].append(
            test_calculations[AVERAGE_LATENCY_TITLE]
        )
        calculations[MAXIMUM_LATENCY_TITLE].append(
            test_calculations[MAXIMUM_LATENCY_TITLE]
        )
        calculations[AVERAGE_EXEC_TITLE].append(test_calculations[AVERAGE_EXEC_TITLE])
        calculations[MAXIMUM_EXEC_TITLE].append(test_calculations[MAXIMUM_EXEC_TITLE])

        test_runs[recorded_name] = new_results
    return main_dictionary


def load_scenario_file():
    """
    Load the contents of the specified suite file.
    """

    if len(sys.argv) != 2:
        print("Suite file must be specified as the second parameter.")
        sys.exit(1)
    suite_file_name = sys.argv[1]
    if not os.path.exists(suite_file_name) or os.path.isdir(suite_file_name):
        print(f"Suite file '{suite_file_name}' must exist and not be a directory.")
        sys.exit(1)
    with open(suite_file_name) as suite_file:
        suite_file_lines = suite_file.readlines()
    return suite_file_lines


def dump_results_dictionary(full_test_results):
    """
    Dump the full_test_results dictionary as a JSON file.
    """
    with open(SUITE_DIRECTORY + "summary.json", "w") as write_file:
        json.dump(full_test_results, write_file, indent=4)


def execute_suite_tests(suite_file_lines):
    """
    Execute each of the tests specified by the lines in the suite file.
    """
    full_test_results = {}
    for next_suite_test in suite_file_lines:

        next_suite_test = next_suite_test.strip()
        is_repeat_test = re.search("^(.*) repeat ([0-9]+)$", next_suite_test)
        if is_repeat_test:
            next_suite_test = is_repeat_test.group(1)
            max_test = int(is_repeat_test.group(2))
            if max_test < 1:
                print(
                    f"Repeat number for test {next_suite_test} must be greater than "
                    + "or equal to 1, not '{max_test}'."
                )
                sys.exit(1)
            full_test_results[next_suite_test] = summarize_repeated_tests(
                next_suite_test, max_test
            )
        else:
            full_test_results[next_suite_test] = load_results_for_test(next_suite_test)
    return full_test_results


file_lines = load_scenario_file()
test_results = execute_suite_tests(file_lines)
dump_results_dictionary(test_results)
