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
stats_log_interval = 2
log_individual_rule_stats = true
rule_retry_count = 3
"""


def load_configuration_values_from_json_file():
    """
    Load any requested configuration values from the JSON file.
    """
    thread_pool_count = -1

    if len(sys.argv) == 2:
        config_json_file = sys.argv[1]
        if not os.path.exists(config_json_file) or os.path.isdir(config_json_file):
            print(
                f"Configuration source file '{config_json_file}' must exist and not be a directory."
            )
            sys.exit(1)
        with open(config_json_file) as input_file:
            data = json.load(input_file)

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


def write_templated_output(thread_count):
    """
    Realize the templated output and write it to the configuration file.
    """

    templated_file_contents = CONFIGURATION_FILE_TEMPLATE.replace(
        "{THREAD_POOL_COUNT}", str(thread_count)
    )

    with open("incubator.conf", "w") as write_file:
        write_file.write(templated_file_contents)


thread_count_value = load_configuration_values_from_json_file()
write_templated_output(thread_count_value)
