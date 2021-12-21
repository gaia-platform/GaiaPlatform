#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to translated the coverage log files into JSON files.
"""

import os
import json
import sys

__BASE_DIR = "output"


def read_coverage_log_file(relative_file_name):
    """
    Read the coverage log file and construct a dictionary with a summary
    of the information contained within that file.
    """
    log_path = os.path.join(__BASE_DIR, relative_file_name)
    with open(log_path) as input_file:
        filter_file_lines = input_file.readlines()

    local_coverage_dictionary = {}
    filter_file_lines = filter_file_lines[-4:]
    if filter_file_lines[0] != "'Summary coverage rate:\n'":
        sys.exit(1)
    filter_file_lines = filter_file_lines[1:]
    for next_line in filter_file_lines:

        next_line = next_line.strip().split(" ")
        title = next_line[0][0:-1].strip(".")
        covered = int(next_line[2][1:])
        total = int(next_line[4])

        stats_dictionary = {}
        stats_dictionary["covered"] = covered
        stats_dictionary["total"] = total
        stats_dictionary["percent"] = round((100.0 * covered) / total, 2)
        local_coverage_dictionary[title] = stats_dictionary

    new_dictionary = {}
    new_dictionary["coverage"] = local_coverage_dictionary
    return new_dictionary


total_coverage_dictionary = read_coverage_log_file("filter.log")
rules_coverage_dictionary = read_coverage_log_file("rules.log")
database_coverage_dictionary = read_coverage_log_file("database.log")
other_coverage_dictionary = read_coverage_log_file("other.log")

coverage_dictionary = {}
coverage_dictionary["total"] = total_coverage_dictionary
coverage_dictionary["rules"] = rules_coverage_dictionary
coverage_dictionary["database"] = database_coverage_dictionary
coverage_dictionary["other"] = other_coverage_dictionary
print(json.dumps(coverage_dictionary, indent=4))
