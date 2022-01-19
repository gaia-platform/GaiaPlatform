#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Script to translate a test's output.json file into a output.csv file.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import json
import sys

convert_paths = [
    "chicken.sensors.0.value",
    "chicken.sensors.1.value",
    "puppy.sensors.0.value",
    "chicken.actuators.0.value",
    "puppy.actuators.0.value",
    "puppy.actuators.0.value",
]

__DEFAULT_FILE_ENCODING = "utf-8"


def traverse(json_data, json_path):
    """
    Traverse the object, using the path to guide they way.
    """
    split_path = json_path.split(".")
    if isinstance(json_data, dict):
        referenced_data = json_data[split_path[0]]
        traversed_data = traverse(referenced_data, ".".join(split_path[1:]))
    elif isinstance(json_data, list):
        value_as_integer = int(split_path[0])
        referenced_data = json_data[value_as_integer]
        traversed_data = traverse(referenced_data, ".".join(split_path[1:]))
    elif isinstance(json_data, float):
        traversed_data = json_data
    else:
        assert False
    return traversed_data


def process_script_action():
    """
    Process the translation of the output.json file into an output.csv file.
    """
    if len(sys.argv) != 2:
        print("Input file must be specified as the second parameter.")
        sys.exit(1)
    input_file_name = sys.argv[1]

    with open(input_file_name, encoding=__DEFAULT_FILE_ENCODING) as input_file:
        data_dictionary = json.load(input_file)

    csv_lines = []
    for next_path in convert_paths:
        csv_lines.append(next_path)

    for next_sample in data_dictionary:
        for index, next_path in enumerate(convert_paths):
            final_object = traverse(next_sample, next_path)
            line_so_far = csv_lines[index] + "," + str(final_object)
            csv_lines[index] = line_so_far

    for next_line in csv_lines:
        print(next_line)


sys.exit(process_script_action())
