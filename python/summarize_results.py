#! /usr/bin/python3
import json
import sys
import os
import re

"""
Module to caluldate the summary for a suite of tests that were run.
"""

suite_directory = "suite-results/"

thread_load_index = 5
scheduled_index = 7
invoked_index = 8
pending_index = 9
abandoned_index = 10
retry_index = 11
exception_index = 12
average_latency_index = 13
maximum_latency_index = 15
average_exec_index = 17
maximum_exec_index = 19

thread_load_title = "thread-load-percent"

iterations_title = "iterations"
return_code_title = "return-code"
test_duration_title = "test-duration-sec"
iteration_duration_title = "iteration-duration-sec"
total_duration_title = "duration-sec"
pause_duration_title = "stop-pause-sec"
wait_duration_title = "wait-pause-sec"
print_duration_title = "print-duration-sec"
test_runs_title = "test_runs"

rules_engine_title = "rules-engine-stats"
rules_engine_slices_title = "slices"
rules_engine_totals_title = "totals"
rules_engine_calculations_title = "calculations"

scheduled_title = "scheduled"
invoked_title = "invoked"
pending_title = "pending"
abandoned_title = "abandoned"
retry_title = "retry"
exception_title = "exception"

average_latency_title = "avg-latency-ms"
maximum_latency_title = "max-latency-ms"
average_exec_title = "avg-exec-ms"
maximum_exec_title = "max-exec-ms"

def calculate_total_counts(stats_slice, log_line_columns, totals, title, index):
    stats_slice[title] = int(log_line_columns[index])
    previous_value = 0
    if title in totals:
        previous_value = totals[title]
    totals[title] = previous_value + stats_slice[title]

def calculate_maximum_values(stats_slice, log_line_columns, calculations, title, index):
    stats_slice[title] = float(log_line_columns[index])
    previous_maximum = 0.0
    if title in calculations:
        previous_maximum = calculations[title]
    calculations[title] = max(previous_maximum, stats_slice[title])

def calculate_average_values(stats_slice, log_line_columns, calculations, title, index):
    stats_slice[title] = float(log_line_columns[index])
    previous_value = 0
    if title in calculations:
        previous_value = calculations[title]
    calculations[title] = previous_value + (stats_slice[title] * stats_slice[scheduled_title])

def process_rules_engine_stats(log_path):
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
        assert len(log_line_columns) == maximum_exec_index + 2, str(log_line_columns)

        stats_slice = {}
        stats_slice[thread_load_title] = float(log_line_columns[thread_load_index])
        calculate_total_counts(stats_slice, log_line_columns, totals, scheduled_title, scheduled_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, invoked_title, invoked_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, pending_title, pending_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, abandoned_title, abandoned_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, retry_title, retry_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, exception_title, exception_index)
        calculate_average_values(stats_slice, log_line_columns, calculations, average_latency_title, average_latency_index)
        calculate_maximum_values(stats_slice, log_line_columns, calculations, maximum_latency_title, maximum_latency_index)
        calculate_average_values(stats_slice, log_line_columns, calculations, average_exec_title, average_exec_index)
        calculate_maximum_values(stats_slice, log_line_columns, calculations, maximum_exec_title, maximum_exec_index)
        stats_slices.append(stats_slice)

    calculations[average_latency_title] = calculations[average_latency_title] / float(totals[scheduled_title])
    calculations[average_exec_title] = calculations[average_exec_title] / float(totals[scheduled_title])

    rules_engine_stats = {}
    rules_engine_stats[rules_engine_slices_title] = stats_slices
    rules_engine_stats[rules_engine_totals_title] = totals
    rules_engine_stats[rules_engine_calculations_title] = calculations
    return rules_engine_stats

def load_test_result_files(suite_test):

    base_dir = os.path.join(suite_directory, suite_test)

    json_path = os.path.join(base_dir, "return_code.json")
    with open(json_path) as input_file:
        data = json.load(input_file)
        return_code_data = data[return_code_title]

    json_path = os.path.join(base_dir, "duration.json")
    with open(json_path) as input_file:
        data = json.load(input_file)
        duration_data = data["duration"]

    json_path = os.path.join(base_dir, "output.delay")
    with open(json_path) as input_file:
        data = json.load(input_file)
        stop_pause_data = data["stop_pause_in_sec"]
        iterations_data = data["iterations"]
        total_wait_data = data["total_wait_in_sec"]
        total_print_data = data["total_print_in_sec"]

    log_path = os.path.join(base_dir, "gaia_stats.log")
    stats_data = process_rules_engine_stats(log_path)

    return return_code_data, duration_data, stop_pause_data, stats_data, iterations_data, total_wait_data, total_print_data

def load_results_for_test(suite_test):
    return_code_data, duration_data, stop_pause_data, stats_data, iterations_data, total_wait_data, total_print_data = load_test_result_files(suite_test)

    new_results = {}
    new_results[iterations_title] = iterations_data
    new_results[return_code_title] = return_code_data
    new_results[total_duration_title] = duration_data
    new_results[pause_duration_title] = stop_pause_data
    new_results[wait_duration_title] = total_wait_data
    new_results[print_duration_title] = total_print_data

    new_results[test_duration_title] =  new_results[total_duration_title] - new_results[pause_duration_title] - new_results[wait_duration_title] - new_results[print_duration_title]
    new_results[iteration_duration_title] = new_results[test_duration_title] / float(new_results[iterations_title])
    new_results[rules_engine_title] = stats_data
    return new_results

def summarize_repeated_tests():

    xx = {}

    xx[iterations_title] = []
    xx[return_code_title] = []
    xx[test_duration_title] = []
    xx[iteration_duration_title] = []

    totals = {}
    totals[scheduled_title] = []
    totals[invoked_title] = []
    totals[pending_title] = []
    totals[abandoned_title] = []
    totals[retry_title] = []
    totals[exception_title] = []

    calculations = {}
    calculations[average_latency_title] = []
    calculations[maximum_latency_title] = []
    calculations[average_exec_title] = []
    calculations[maximum_exec_title] = []

    rules_engine_stats = {}
    rules_engine_stats[rules_engine_totals_title] = totals
    rules_engine_stats[rules_engine_calculations_title] = calculations

    xx[rules_engine_title] = rules_engine_stats

    test_runs = {}
    xx[test_runs_title] = test_runs
    for test_repeat in range(1, max_test + 1):
        recorded_name = suite_test + "_" + str(test_repeat)
        new_results = load_results_for_test(recorded_name)

        xx[iterations_title].append(new_results[iterations_title])
        xx[return_code_title].append(new_results[return_code_title])
        xx[test_duration_title].append(new_results[test_duration_title])
        xx[iteration_duration_title].append(new_results[iteration_duration_title])

        test_totals = new_results[rules_engine_title][rules_engine_totals_title]
        totals[scheduled_title].append(test_totals[scheduled_title])
        totals[invoked_title].append(test_totals[invoked_title])
        totals[pending_title].append(test_totals[pending_title])
        totals[abandoned_title].append(test_totals[abandoned_title])
        totals[retry_title].append(test_totals[retry_title])
        totals[exception_title].append(test_totals[exception_title])

        test_calculations = new_results[rules_engine_title][rules_engine_calculations_title]
        calculations[average_latency_title].append(test_calculations[average_latency_title])
        calculations[maximum_latency_title].append(test_calculations[maximum_latency_title])
        calculations[average_exec_title].append(test_calculations[average_exec_title])
        calculations[maximum_exec_title].append(test_calculations[maximum_exec_title])

        test_runs[recorded_name] = new_results
    return xx

if len(sys.argv) != 2:
    print("error")
    exit(1)
suite_file = sys.argv[1]
if not os.path.exists(suite_file) or os.path.isdir(suite_file):
    print("is not file")
    exit(1)

with open(suite_file) as input_file:
    suite_file_lines = input_file.readlines()

full_test_results = {}
for suite_test in suite_file_lines:

    suite_test = suite_test.strip()
    is_repeat_test = re.search("^(.*) repeat ([0-9]+)$", suite_test)
    if is_repeat_test:
        suite_test = is_repeat_test.group(1)
        max_test = int(is_repeat_test.group(2))
        full_test_results[suite_test] = summarize_repeated_tests()
    else:
        full_test_results[suite_test] = load_results_for_test(suite_test)

with open(suite_directory + "summary.json", "w") as write_file:
    json.dump(full_test_results, write_file, indent=4)
