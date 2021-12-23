#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Script to translate a test's output.json file into a output.csv file.
"""

import json
import sys
import os
import argparse

__DEFAULT_FILE_ENCODING = "utf-8"


def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Generate statistics based on the various suites that were executed."
    )
    parser.add_argument(
        "--directory",
        dest="source_directory",
        action="store",
        required=True,
        help="Directory to grab any summaries from.",
    )
    parser.add_argument(
        "--output",
        dest="output_file_name",
        action="store",
        help="File name to write the summaries to.",
    )
    return parser.parse_args()


def __read_source_json(args, next_directory_name):
    suite_summary_file = os.path.join(
        args.source_directory, next_directory_name, "summary.json"
    )
    if not os.path.exists(suite_summary_file):
        print(f"Suite summary file '{suite_summary_file}' does not exist.")
        return 1
    suite_stats_file = os.path.join(
        args.source_directory, next_directory_name, "suite-stats.json"
    )
    if not os.path.exists(suite_stats_file):
        print(f"Suite stats file '{suite_stats_file}' does not exist.")
        return 1

    with open(suite_summary_file, encoding=__DEFAULT_FILE_ENCODING) as input_file:
        data_dictionary = json.load(input_file)
    with open(suite_stats_file, encoding=__DEFAULT_FILE_ENCODING) as input_file:
        stats_dictionary = json.load(input_file)
    return data_dictionary, stats_dictionary


def __process_peformance_result(
    test_output_dictionary, test_legend, test_legend_summary
):
    for next_legend_name in test_legend:
        test_output_dictionary["measurements"][next_legend_name] = test_legend_summary[
            next_legend_name
        ]
        test_output_dictionary["measurements"][next_legend_name][
            "description"
        ] = test_legend[next_legend_name]


def __process_integration_result(
    test_return_codes,
    failed_integration_tests,
    next_directory_name,
    next_test_name,
    is_list_measurement,
):
    if is_list_measurement:
        for i in test_return_codes:
            if i:
                failed_integration_tests.append(
                    next_directory_name + "." + next_test_name
                )
                break
    else:
        if test_return_codes:
            failed_integration_tests.append(next_directory_name + "." + next_test_name)


# pylint: disable=too-many-arguments)
def __process_suite_results(
    data_dictionary,
    stats_dictionary,
    next_directory_name,
    total_performance_tests,
    total_integration_tests,
    failed_integration_tests,
):
    suite_output_dictionary = {}
    for next_test_name in data_dictionary:
        if "configuration" in data_dictionary[next_test_name]:
            thread_count = data_dictionary[next_test_name]["configuration"][
                "thread_pool_count"
            ]
        else:
            for i in data_dictionary[next_test_name]["test_runs"]:
                thread_count = data_dictionary[next_test_name]["test_runs"][i][
                    "configuration"
                ]["thread_pool_count"]
                break

        test_type = data_dictionary[next_test_name]["properties"]["test-type"]

        test_description = next_test_name
        if "description" in data_dictionary[next_test_name]["properties"]:
            test_description = data_dictionary[next_test_name]["properties"][
                "description"
            ]

        test_output_dictionary = {}
        number_of_iterations = data_dictionary[next_test_name]["per-test"]["iterations"]
        is_list_measurement = isinstance(number_of_iterations, list)
        test_output_dictionary["suite-name"] = next_directory_name
        test_output_dictionary["suite-workload"] = stats_dictionary[next_test_name][
            "workload-name"
        ]
        test_output_dictionary["suite-test"] = next_test_name
        test_output_dictionary["test-type"] = test_type
        test_output_dictionary["test-description"] = test_description

        test_output_dictionary["conifguration"] = {}
        test_output_dictionary["conifguration"]["thread-count"] = thread_count

        if is_list_measurement:
            test_output_dictionary["conifguration"][
                "iterations"
            ] = number_of_iterations[0]
        else:
            test_output_dictionary["conifguration"]["iterations"] = number_of_iterations
        test_output_dictionary["conifguration"]["run-return-code"] = data_dictionary[
            next_test_name
        ]["per-test"]["return-code"]
        test_output_dictionary["conifguration"]["test-return-code"] = data_dictionary[
            next_test_name
        ]["per-test"]["invoke-return-code"]

        test_output_dictionary["measurements"] = {}
        test_output_dictionary["measurements"][
            "total-test-duration-secs"
        ] = data_dictionary[next_test_name]["per-test"]["measured-duration-sec"]

        if test_type == "performance":
            total_performance_tests += 1
            __process_peformance_result(
                test_output_dictionary,
                data_dictionary[next_test_name]["x-legend"],
                stats_dictionary[next_test_name]["summary"],
            )
        else:
            total_integration_tests += 1
            __process_integration_result(
                test_output_dictionary["conifguration"]["test-return-code"],
                failed_integration_tests,
                next_directory_name,
                next_test_name,
                is_list_measurement,
            )

        suite_output_dictionary[next_test_name] = test_output_dictionary
    return suite_output_dictionary, total_performance_tests, total_integration_tests


# pylint: enable=too-many-arguments)


def __process_script_action():
    """
    Process the posting of the message.
    """

    args = __process_command_line()
    if not os.path.exists(args.source_directory):
        print(f"Source directory '{args.source_directory}' does not exist.")
        return 1
    if not os.path.isdir(args.source_directory):
        print(f"Source directory '{args.source_directory}' is not a directory.")
        return 1

    total_integration_tests = 0
    total_performance_tests = 0
    failed_integration_tests = []

    full_output_dictionary = {}
    only_directories = [
        file_name
        for file_name in os.listdir(args.source_directory)
        if os.path.isdir(os.path.join(args.source_directory, file_name))
    ]
    for next_directory_name in only_directories:
        data_dictionary, stats_dictionary = __read_source_json(
            args, next_directory_name
        )

        (
            suite_output_dictionary,
            total_performance_tests,
            total_integration_tests,
        ) = __process_suite_results(
            data_dictionary,
            stats_dictionary,
            next_directory_name,
            total_performance_tests,
            total_integration_tests,
            failed_integration_tests,
        )
        full_output_dictionary[next_directory_name] = suite_output_dictionary
    if args.output_file_name:
        with open(
            args.output_file_name, "wt", encoding=__DEFAULT_FILE_ENCODING
        ) as write_file:
            json.dump(full_output_dictionary, write_file, indent=4)
    else:
        print(json.dumps(full_output_dictionary, indent=4))

    if total_performance_tests:
        print(f"Completed executing {total_performance_tests} performance tests.")
    if total_integration_tests:
        print(f"Completed executing {total_integration_tests} integration tests.")

    if failed_integration_tests:
        print(
            f"There were {len(failed_integration_tests)} failures in integration tests:"
        )
        for next_test in failed_integration_tests:
            print(f"  {next_test}")
        return 1
    return 0


sys.exit(__process_script_action())
