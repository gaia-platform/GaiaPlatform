#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Script to generate a Gaia configuration file from a more compact config.json file.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import sys
import os
import json
import argparse

CONFIGURATION_FILE_TEMPLATE = """
[Database]
data_dir = "/var/lib/gaia/db"
instance_name="gaia_default_instance"
[Catalog]
[Rules]
thread_pool_count = {THREAD_POOL_COUNT}
stats_log_interval = {STATS_LOG_INTERVAL}
log_individual_rule_stats = true
rule_retry_count = 3
"""


def check_non_negative(value):
    """
    Verify that the input value in a non-negative integer value.
    """
    integer_value = int(value)
    if integer_value < 0:
        raise argparse.ArgumentTypeError(
            f"Parameter {value} is not a non-negative integer value."
        )
    return integer_value


def process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Construct a Gaia configuration file from a JSON blob."
    )
    parser.add_argument(
        "--config",
        dest="config",
        action="store",
        help="Configuration file to use as a starting point.",
    )
    parser.add_argument(
        "--threads",
        dest="threads",
        action="store",
        type=check_non_negative,
        help="Number of threads to use when executing.  Note that a value of `0` "
        + "equals as many threads as possible.",
    )
    parser.add_argument(
        "--output",
        dest="output_file_name",
        action="store",
        help="Output file name for the generated configuration file.",
    )
    return parser.parse_args()


def read_thread_pool_count(data, thread_pool_count):
    """
    Retrive the thread_pool_count setting from the configuration file.
    """
    if "thread_pool_count" in data:
        thread_pool_count = data["thread_pool_count"]
        if not isinstance(thread_pool_count, int):
            print(
                "Configuration source file's 'thread_pool_count' element was not an integer."
            )
            sys.exit(1)
        elif thread_pool_count < 1 and thread_pool_count != -1:
            print(
                "Configuration source file's 'thread_pool_count' element must either be "
                + "a positive integer or -1."
            )
            sys.exit(1)
    return thread_pool_count


def read_stats_log_interval(data, stats_log_interval):
    """
    Retrive the stats_log_interval setting from the configuration file.
    """
    if "stats_log_interval" in data:
        stats_log_interval = data["stats_log_interval"]
        if not isinstance(stats_log_interval, int):
            print(
                "Configuration source file's 'stats_log_interval' element was not an integer."
            )
            sys.exit(1)
        elif stats_log_interval < 1 or stats_log_interval > 300:
            print(
                "Configuration source file's 'stats_log_interval' element must be "
                + "between (and including) 1 and 300."
            )
            sys.exit(1)
    return stats_log_interval


def load_configuration_values_from_json_file(args):
    """
    Load any requested configuration values from the JSON file.
    """
    thread_pool_count = -1
    stats_log_interval = 2

    if args.config:
        config_json_file = args.config
        if not os.path.exists(config_json_file) or os.path.isdir(config_json_file):
            print(
                f"Configuration source file '{config_json_file}' must exist and not be a directory."
            )
            sys.exit(1)
        with open(config_json_file) as input_file:
            data = json.load(input_file)

        thread_pool_count = read_thread_pool_count(data, thread_pool_count)
        stats_log_interval = read_stats_log_interval(data, stats_log_interval)

    # If a thread value was specified on the command line, it takes
    # precedence over any configuration file that was loaded.  Note that
    # on the command line, a thread parameter of `0` is translated into
    # the engine's `-1`.
    if args.threads is not None:
        thread_pool_count = args.threads if args.threads else -1

    return thread_pool_count, stats_log_interval


def write_templated_output(output_file_name, thread_count, stats_log_interval):
    """
    Realize the templated output and write it to the configuration file.
    """

    templated_file_contents = CONFIGURATION_FILE_TEMPLATE.replace(
        "{THREAD_POOL_COUNT}", str(thread_count)
    ).replace("{STATS_LOG_INTERVAL}", str(stats_log_interval))

    with open(output_file_name, "w") as write_file:
        write_file.write(templated_file_contents)


def process_script_action():
    """
    Process the posting of the message.
    """
    args = process_command_line()
    thread_count_value, stats_log_interval = load_configuration_values_from_json_file(
        args
    )
    write_templated_output(
        args.output_file_name, thread_count_value, stats_log_interval
    )
    print(str(stats_log_interval))


sys.exit(process_script_action())
