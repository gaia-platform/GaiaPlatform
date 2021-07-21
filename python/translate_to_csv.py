#! /usr/bin/python3

"""
Script to translate a test's output.json file into a output.csv file.

Copyright (c) Gaia Platform LLC
All rights reserved.
"""

import json

convert_paths = [
    "chicken.sensors.0.value",
    "chicken.sensors.1.value",
    "puppy.sensors.0.value",
    "chicken.actuators.0.value",
    "puppy.actuators.0.value",
    "puppy.actuators.0.value"
    ]

def traverse(json_data, json_path):
    """
    Traverse the object, using the path to guide they way.
    """
    split_path = json_path.split(".")
    if isinstance(json_data, dict):
        referenced_data = json_data[split_path[0]]
        traversed_data = traverse(referenced_data, ".".join(split_path[1:]))
    elif isinstance(json_data, list):
        rt = int(split_path[0])
        referenced_data = json_data[rt]
        traversed_data = traverse(referenced_data, ".".join(split_path[1:]))
    elif isinstance(json_data, float):
        traversed_data = json_data
    else:
        assert False
    return traversed_data

with open("build/output.json") as input_file:
    incubator_data = json.load(input_file)

csv_lines = []
for next_path in convert_paths:
    csv_lines.append(next_path)

for next_incubator_sample in incubator_data:
    for index,next_path in enumerate(convert_paths):
        final_object = traverse(next_incubator_sample,next_path)
        line_so_far = csv_lines[index] + "," + str(final_object)
        csv_lines[index] = line_so_far

for next_line in csv_lines:
    print(next_line)
