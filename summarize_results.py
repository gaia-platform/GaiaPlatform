#! /usr/bin/python3
import json
import sys
import os

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

def process_rules_engine_stats(log_path):
    with open(log_path) as input_file:
        log_file_lines = input_file.readlines()

    stats_slices = []
    totals = {}
    for next_log_line in log_file_lines:
        next_log_line = next_log_line.strip()
        while "  " in next_log_line:
            next_log_line = next_log_line.replace("  ", " ")
        log_line_columns = next_log_line.split(" ")
        if log_line_columns[3].startswith("--"):
            continue
        assert len(log_line_columns) == maximum_exec_index + 2

        stats_slice = {}
        stats_slice[thread_load_title] = float(log_line_columns[thread_load_index])
        calculate_total_counts(stats_slice, log_line_columns, totals, scheduled_title, scheduled_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, invoked_title, invoked_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, pending_title, pending_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, abandoned_title, abandoned_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, retry_title, retry_index)
        calculate_total_counts(stats_slice, log_line_columns, totals, exception_title, exception_index)
        stats_slice[average_latency_title] = float(log_line_columns[average_latency_index])
        stats_slice[maximum_latency_title] = float(log_line_columns[maximum_latency_index])
        stats_slice[average_exec_title] = float(log_line_columns[average_exec_index])
        stats_slice[maximum_exec_title] = float(log_line_columns[maximum_exec_index])
        stats_slices.append(stats_slice)

    rules_engine_stats = {}
    rules_engine_stats["slices"] = stats_slices
    rules_engine_stats["totals"] = totals
    return rules_engine_stats

def load_test_result_files(suite_test):
    base_dir = os.path.join(suite_directory, suite_test)

    json_path = os.path.join(base_dir, "return_code.json")
    with open(json_path) as input_file:
        return_code_data = json.load(input_file)
        return_code_data = return_code_data["return-code"]

    json_path = os.path.join(base_dir, "duration.json")
    with open(json_path) as input_file:
        duration_data = json.load(input_file)
        duration_data = duration_data["duration"]

    json_path = os.path.join(base_dir, "output.delay")
    with open(json_path) as input_file:
        stop_pause_data = json.load(input_file)
        stop_pause_data = stop_pause_data["stop_pause_in_sec"]

    log_path = os.path.join(base_dir, "gaia_stats.log")
    stats_data = process_rules_engine_stats(log_path)

    return return_code_data, duration_data, stop_pause_data, stats_data

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
    return_code_data, duration_data, stop_pause_data, stats_data = load_test_result_files(suite_test)

    new_results = {}
    new_results["return-code"] = return_code_data
    new_results["duration-sec"] = duration_data
    new_results["stop-pause-sec"] = stop_pause_data
    new_results["test-duration-sec"] =  new_results["duration-sec"] - new_results["stop-pause-sec"]
    new_results["rules-engine-stats"] = stats_data

    full_test_results[suite_test] = new_results

with open(suite_directory + "summary.json", "w") as write_file:
    json.dump(full_test_results, write_file, indent=4)
