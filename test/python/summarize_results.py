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

DECIMALS_PLACES_IN_NANOSECONDS = 9

SLICE_NAME_INDEX = 3
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

AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA = -2

THREAD_LOAD_TITLE = "thread-load-percent"

SOURCE_TITLE = "source"
CONFIGURATION_TITLE = "configuration"
ITERATIONS_TITLE = "iterations"
RETURN_CODE_TITLE = "return-code"
TEST_DURATION_TITLE = "test-duration-sec"
ITERATION_DURATION_TITLE = "iteration-duration-sec"
MEASURED_DURATION_TITLE = "measured-duration-sec"
PER_MEASURED_DURATION_TITLE = "iteration-measured-duration-sec"
TOTAL_DURATION_TITLE = "duration-sec"
PAUSE_DURATION_TITLE = "stop-pause-sec"
WAIT_DURATION_TITLE = "wait-pause-sec"
PRINT_DURATION_TITLE = "print-duration-sec"
TEST_RUNS_TITLE = "test_runs"

T_PAUSE_TITLE = "t-requested-pause-microseconds"
T_OVER_PERCENT_TITLE = "t-over-percent"
T_PAUSE_SECONDS_TITLE = "t-pause-sec"
T_REQUESTED_SECONDS_TITLE = "t-requested-sec"
T_EXCESS_NANOSECONDS_TITLE = "t-excess-microsec"
T_PER_EXCESS_NANOSECONDS_TITLE = "t-individual-excess-microsec"

SOURCE_FILE_NAME_TITLE = "file_name"
SOURCE_LINE_NUMBER_TITLE = "line_number"

INDIVIDUAL_STATS_TITLE = "individual"

RULES_ENGINE_TITLE = "rules-engine-stats"
RULES_ENGINE_SLICES_TITLE = "slices"
RULES_ENGINE_TOTALS_TITLE = "totals"
RULES_ENGINE_CALCULATIONS_TITLE = "calculations"
RULES_ENGINE_EXCEPTIONS_TITLE = "logged-exceptions"

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

OBJECT_ID_TITLE = "rule-line-number"
OBJECT_NAME_TITLE = "rule-additional-name"


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


def process_indivudal_stats_line(log_line_columns, totals, calculations):
    """
    Given a line of a gaia_stats.log file that is not an aggregate, break it
    apart and store the information under the aggregate totals.
    """

    individual_stats = {}
    individual_stats[OBJECT_ID_TITLE] = log_line_columns[SLICE_NAME_INDEX]
    individual_stats[OBJECT_NAME_TITLE] = log_line_columns[SLICE_NAME_INDEX + 1]

    if INDIVIDUAL_STATS_TITLE not in totals:
        totals[INDIVIDUAL_STATS_TITLE] = {}
    individual_totals_item = totals[INDIVIDUAL_STATS_TITLE]
    if individual_stats[OBJECT_ID_TITLE] not in individual_totals_item:
        individual_totals_item[individual_stats[OBJECT_ID_TITLE]] = {}
    individual_object_totals_item = individual_totals_item[
        individual_stats[OBJECT_ID_TITLE]
    ]

    calculate_total_counts(
        individual_stats,
        log_line_columns,
        individual_object_totals_item,
        SCHEDULED_TITLE,
        SCHEDULED_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_total_counts(
        individual_stats,
        log_line_columns,
        individual_object_totals_item,
        INVOKED_TITLE,
        INVOKED_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_total_counts(
        individual_stats,
        log_line_columns,
        individual_object_totals_item,
        PENDING_TITLE,
        PENDING_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_total_counts(
        individual_stats,
        log_line_columns,
        individual_object_totals_item,
        ABANDONED_TITLE,
        ABANDONED_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_total_counts(
        individual_stats,
        log_line_columns,
        individual_object_totals_item,
        RETRY_TITLE,
        RETRY_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_total_counts(
        individual_stats,
        log_line_columns,
        individual_object_totals_item,
        EXCEPTION_TITLE,
        EXCEPTION_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )

    if INDIVIDUAL_STATS_TITLE not in calculations:
        calculations[INDIVIDUAL_STATS_TITLE] = {}
    individual_calculations_item = calculations[INDIVIDUAL_STATS_TITLE]
    if individual_stats[OBJECT_ID_TITLE] not in individual_calculations_item:
        individual_calculations_item[individual_stats[OBJECT_ID_TITLE]] = {}
    individual_object_calculations_item = individual_calculations_item[
        individual_stats[OBJECT_ID_TITLE]
    ]

    calculate_average_values(
        individual_stats,
        log_line_columns,
        individual_object_calculations_item,
        AVERAGE_LATENCY_TITLE,
        AVERAGE_LATENCY_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_maximum_values(
        individual_stats,
        log_line_columns,
        individual_object_calculations_item,
        MAXIMUM_LATENCY_TITLE,
        MAXIMUM_LATENCY_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_average_values(
        individual_stats,
        log_line_columns,
        individual_object_calculations_item,
        AVERAGE_EXEC_TITLE,
        AVERAGE_EXEC_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    calculate_maximum_values(
        individual_stats,
        log_line_columns,
        individual_object_calculations_item,
        MAXIMUM_EXEC_TITLE,
        MAXIMUM_EXEC_INDEX + AGGREGATE_TO_INDIVIDUAL_INDEX_DELTA,
    )
    return individual_stats


def process_aggregate_stats_line(log_line_columns, totals, calculations):
    """
    Take care of processing a line beginning with `[thread load:` into an aggregate
    statis slice dictionary.
    """
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
    return stats_slice


def calculate_proper_averages(calculations, totals):
    """
    Once all the data has been collected, the weighted averages can be properly
    calculated.
    """

    calculations[AVERAGE_LATENCY_TITLE] = round(
        calculations[AVERAGE_LATENCY_TITLE] / float(totals[SCHEDULED_TITLE]),
        DECIMALS_PLACES_IN_NANOSECONDS,
    )
    calculations[AVERAGE_EXEC_TITLE] = round(
        calculations[AVERAGE_EXEC_TITLE] / float(totals[SCHEDULED_TITLE]),
        DECIMALS_PLACES_IN_NANOSECONDS,
    )

    individual_totals = totals["individual"]
    individual_calculations = calculations["individual"]
    for next_total_name in individual_totals:
        next_individual_total = individual_totals[next_total_name]
        next_individual_calculation = individual_calculations[next_total_name]

        next_individual_calculation[AVERAGE_LATENCY_TITLE] = round(
            next_individual_calculation[AVERAGE_LATENCY_TITLE]
            / float(next_individual_total[SCHEDULED_TITLE]),
            DECIMALS_PLACES_IN_NANOSECONDS,
        )
        next_individual_calculation[AVERAGE_EXEC_TITLE] = round(
            next_individual_calculation[AVERAGE_EXEC_TITLE]
            / float(next_individual_total[SCHEDULED_TITLE]),
            DECIMALS_PLACES_IN_NANOSECONDS,
        )


def process_rules_engine_logs(base_dir):
    """
    Load up the `gaia.log` file for a given test and specifically
    extract any information about exceptions logged in that log.
    """

    log_path = os.path.join(base_dir, "gaia.log")
    with open(log_path) as input_file:
        log_file_lines = input_file.readlines()

    exception_lines = []
    search_for = "exception: "
    for next_log_line in log_file_lines:
        next_log_line = next_log_line.strip()
        found_index = next_log_line.find(search_for)
        if found_index != -1:
            exception_lines.append(next_log_line[found_index + len(search_for) :])
    return exception_lines


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
        if log_line_columns[3].startswith("[thread"):
            assert len(log_line_columns) == MAXIMUM_EXEC_INDEX + 2, str(
                log_line_columns
            )
            stats_slice = process_aggregate_stats_line(
                log_line_columns, totals, calculations
            )
            stats_slices.append(stats_slice)
        else:
            individual_stats = process_indivudal_stats_line(
                log_line_columns, totals, calculations
            )
            individual_name = individual_stats[OBJECT_ID_TITLE]
            owner_slice = stats_slices[-1]
            if INDIVIDUAL_STATS_TITLE not in owner_slice:
                owner_slice[INDIVIDUAL_STATS_TITLE] = {}
            owner_slice[INDIVIDUAL_STATS_TITLE][individual_name] = individual_stats

    calculate_proper_averages(calculations, totals)

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
        t_pause_data = data["t_pause_in_sec"]
        t_requested_data = data["t_requested_in_sec"]

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
    )


def load_test_result_files(suite_test_directory):
    """
    Load sets of individual results from their various sources.

    The various sources are used because it is easier for each component
    to emit its own data without having to worry about the integrity of
    a single file.
    """

    base_dir = (
        suite_test_directory
        if os.path.isabs(suite_test_directory)
        else os.path.join(SUITE_DIRECTORY, suite_test_directory)
    )
    return_code_data, duration_data = load_simple_result_files(base_dir)
    (
        stop_pause_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        measured_section_data,
        t_pause_data,
        t_requested_data,
        t_config_data,
    ) = load_output_timing_files(base_dir)

    stats_data = process_rules_engine_stats(base_dir)

    log_data = process_rules_engine_logs(base_dir)

    configuration_data = process_configuration_file(base_dir)

    return (
        return_code_data,
        duration_data,
        stop_pause_data,
        stats_data,
        log_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        measured_section_data,
        configuration_data,
        t_pause_data,
        t_requested_data,
        t_config_data,
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


# pylint: disable=too-many-locals
def load_results_for_test(suite_test_directory, source_info):
    """
    Load all the results for tests and place them in the main dictionary.
    """

    (
        return_code_data,
        duration_data,
        stop_pause_data,
        stats_data,
        log_data,
        iterations_data,
        total_wait_data,
        total_print_data,
        measured_section_data,
        configuration_data,
        t_pause_data,
        t_requested_data,
        t_config_data,
    ) = load_test_result_files(suite_test_directory)

    new_results = {}
    if source_info:
        new_results[SOURCE_TITLE] = source_info
    new_results[CONFIGURATION_TITLE] = configuration_data
    new_results[ITERATIONS_TITLE] = iterations_data
    new_results[RETURN_CODE_TITLE] = return_code_data
    new_results[TOTAL_DURATION_TITLE] = duration_data
    new_results[PAUSE_DURATION_TITLE] = stop_pause_data
    new_results[WAIT_DURATION_TITLE] = total_wait_data
    new_results[PRINT_DURATION_TITLE] = total_print_data
    __handle_t_data(
        new_results, t_pause_data, t_requested_data, t_config_data, iterations_data
    )

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

    new_results[RULES_ENGINE_TITLE] = stats_data
    new_results[RULES_ENGINE_EXCEPTIONS_TITLE] = log_data
    return new_results


# pylint: enable=too-many-locals


def summarize_repeated_tests(max_test, map_lines, map_line_index, source_info):
    """
    Create a summary dictionary for any repeated tests.
    """

    main_dictionary = {}

    main_dictionary[SOURCE_TITLE] = source_info
    main_dictionary[ITERATIONS_TITLE] = []
    main_dictionary[RETURN_CODE_TITLE] = []
    main_dictionary[TEST_DURATION_TITLE] = []
    main_dictionary[MEASURED_DURATION_TITLE] = []

    main_dictionary[ITERATION_DURATION_TITLE] = []
    main_dictionary[PER_MEASURED_DURATION_TITLE] = []
    main_dictionary[T_OVER_PERCENT_TITLE] = []

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
    for _ in range(1, max_test + 1):
        recorded_name = map_lines[map_line_index].strip()
        map_line_index += 1
        new_results = load_results_for_test(recorded_name, None)

        add_individual_test_results(main_dictionary, new_results, totals, calculations)

        test_runs[recorded_name] = new_results

    test_value = main_dictionary[T_OVER_PERCENT_TITLE]
    if not test_value:
        del main_dictionary[T_OVER_PERCENT_TITLE]
    return main_dictionary, map_line_index


def add_individual_test_results(main_dictionary, new_results, totals, calculations):
    """
    Add the approriate fields to the main aggregation list for that field.
    """

    main_dictionary[ITERATIONS_TITLE].append(new_results[ITERATIONS_TITLE])
    main_dictionary[RETURN_CODE_TITLE].append(new_results[RETURN_CODE_TITLE])
    main_dictionary[TEST_DURATION_TITLE].append(new_results[TEST_DURATION_TITLE])
    if MEASURED_DURATION_TITLE in new_results:
        main_dictionary[MEASURED_DURATION_TITLE].append(
            new_results[MEASURED_DURATION_TITLE]
        )
    if ITERATION_DURATION_TITLE in new_results:
        main_dictionary[ITERATION_DURATION_TITLE].append(
            new_results[ITERATION_DURATION_TITLE]
        )
    if PER_MEASURED_DURATION_TITLE in new_results:
        main_dictionary[PER_MEASURED_DURATION_TITLE].append(
            new_results[PER_MEASURED_DURATION_TITLE]
        )
    if T_OVER_PERCENT_TITLE in new_results:
        main_dictionary[T_OVER_PERCENT_TITLE].append(new_results[T_OVER_PERCENT_TITLE])

    test_totals = new_results[RULES_ENGINE_TITLE][RULES_ENGINE_TOTALS_TITLE]
    totals[SCHEDULED_TITLE].append(test_totals[SCHEDULED_TITLE])
    totals[INVOKED_TITLE].append(test_totals[INVOKED_TITLE])
    totals[PENDING_TITLE].append(test_totals[PENDING_TITLE])
    totals[ABANDONED_TITLE].append(test_totals[ABANDONED_TITLE])
    totals[RETRY_TITLE].append(test_totals[RETRY_TITLE])
    totals[EXCEPTION_TITLE].append(test_totals[EXCEPTION_TITLE])

    test_calculations = new_results[RULES_ENGINE_TITLE][RULES_ENGINE_CALCULATIONS_TITLE]
    calculations[AVERAGE_LATENCY_TITLE].append(test_calculations[AVERAGE_LATENCY_TITLE])
    calculations[MAXIMUM_LATENCY_TITLE].append(test_calculations[MAXIMUM_LATENCY_TITLE])
    calculations[AVERAGE_EXEC_TITLE].append(test_calculations[AVERAGE_EXEC_TITLE])
    calculations[MAXIMUM_EXEC_TITLE].append(test_calculations[MAXIMUM_EXEC_TITLE])


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
    return suite_file_name, suite_file_lines


def load_execution_map_file():
    """
    Load the contents of the generated execution map file.
    """

    execution_map_file_name = f"{SUITE_DIRECTORY}map.txt"
    with open(execution_map_file_name) as suite_file:
        map_file_lines = suite_file.readlines()
    return map_file_lines


def dump_results_dictionary(full_test_results):
    """
    Dump the full_test_results dictionary as a JSON file.
    """
    with open(f"{SUITE_DIRECTORY}summary.json", "w") as write_file:
        json.dump(full_test_results, write_file, indent=4)


def execute_suite_tests(suite_file, suite_file_lines, map_lines):
    """
    Execute each of the tests specified by the lines in the suite file.
    """
    full_test_results = {}
    map_line_index = 0
    file_line_number = 1
    for next_suite_test in suite_file_lines:
        next_suite_test = next_suite_test.strip()

        source_info = {}
        source_info[SOURCE_FILE_NAME_TITLE] = suite_file
        source_info[SOURCE_LINE_NUMBER_TITLE] = file_line_number
        file_line_number += 1

        is_repeat_test = re.search("^#", next_suite_test)
        if is_repeat_test:
            continue

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
            map_line_index += 1

            name_collision_avoidance_index = 1
            original_next_suite_test = next_suite_test
            while next_suite_test in full_test_results:
                next_suite_test = (
                    f"{original_next_suite_test}___{name_collision_avoidance_index}"
                )
                name_collision_avoidance_index += 1

            (
                full_test_results[next_suite_test],
                map_line_index,
            ) = summarize_repeated_tests(
                max_test, map_lines, map_line_index, source_info
            )
            map_line_index += 1
        else:
            suite_test_results_directory = map_lines[map_line_index].strip()
            simple_results_name = os.path.basename(suite_test_results_directory)
            full_test_results[simple_results_name] = load_results_for_test(
                suite_test_results_directory, source_info
            )
            map_line_index += 1

    assert len(map_lines) == map_line_index
    return full_test_results


def process_script_action():
    """
    Process the posting of the message.
    """
    suite_file_name, map_file_lines = load_scenario_file()
    map_lines = load_execution_map_file()
    test_results = execute_suite_tests(suite_file_name, map_file_lines, map_lines)
    dump_results_dictionary(test_results)


sys.exit(process_script_action())
