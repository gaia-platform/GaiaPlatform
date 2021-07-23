#! /usr/bin/python3

"""
Script to generate a Gaia configuration file from a more compact config.json file.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import sys
import os
import json

THREAD_POOL_COUNT = -1

if len(sys.argv) == 2:
    config_json_file = sys.argv[1]
    if not os.path.exists(config_json_file) or os.path.isdir(config_json_file):
        print("is not file")
        sys.exit(1)
    with open(config_json_file) as input_file:
        data = json.load(input_file)

    if "thread_pool_count" in data:
        THREAD_POOL_COUNT = data["thread_pool_count"]

DEFAULT_CONFIGURATION_TEXT = """
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

CONFIGURATION_TEXT = DEFAULT_CONFIGURATION_TEXT.replace(
    "{THREAD_POOL_COUNT}", str(THREAD_POOL_COUNT)
)

with open("incubator.conf", "w") as write_file:
    write_file.write(CONFIGURATION_TEXT)
