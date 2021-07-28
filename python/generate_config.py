#! /usr/bin/python3

"""
Script to generate a Gaia configuration file from a more compact config.json file.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import sys
import os
import json

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

def read_thread_pool_count(data, thread_pool_count):
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
    if "stats_log_interval" in data:
        stats_log_interval = data["stats_log_interval"]
        if not isinstance(stats_log_interval, int):
            print(
                "Configuration source file's 'stats_log_interval' element was not an integer."
            )
            sys.exit(1)
        elif stats_log_interval < 1 or stats_log_interval > 60:
            print(
                "Configuration source file's 'stats_log_interval' element must be "
                + "between (and including) 1 and 60."
            )
            sys.exit(1)
    return stats_log_interval

def load_configuration_values_from_json_file():
    """
    Load any requested configuration values from the JSON file.
    """
    thread_pool_count = -1
    stats_log_interval = 2

    if len(sys.argv) == 2:
        config_json_file = sys.argv[1]
        if not os.path.exists(config_json_file) or os.path.isdir(config_json_file):
            print(
                f"Configuration source file '{config_json_file}' must exist and not be a directory."
            )
            sys.exit(1)
        with open(config_json_file) as input_file:
            data = json.load(input_file)

        thread_pool_count = read_thread_pool_count(data, thread_pool_count)
        stats_log_interval = read_stats_log_interval(data, stats_log_interval)
    return thread_pool_count, stats_log_interval


def write_templated_output(thread_count):
    """
    Realize the templated output and write it to the configuration file.
    """

    templated_file_contents = CONFIGURATION_FILE_TEMPLATE\
    .replace(
        "{THREAD_POOL_COUNT}", str(thread_count)
    ).replace(
        "{STATS_LOG_INTERVAL}", str(stats_log_interval)
    )

    with open("incubator.conf", "w") as write_file:
        write_file.write(templated_file_contents)


thread_count_value, stats_log_interval = load_configuration_values_from_json_file()
write_templated_output(thread_count_value)
print(str(stats_log_interval))
