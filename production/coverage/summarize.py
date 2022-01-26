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

__DEFAULT_FILE_ENCODING = "utf-8"


# pylint: disable=broad-except
def read_coverage_log_file(relative_file_name):
    """
    Read the coverage log file and construct a dictionary with a summary
    of the information contained within that file.
    """
    try:
        log_path = os.path.join(__BASE_DIR, relative_file_name)
        with open(log_path, encoding=__DEFAULT_FILE_ENCODING) as input_file:
            new_dictionary = json.load(input_file)
    except Exception as this_exception:
        print(f"Unable to load file {relative_file_name}: {this_exception}")
    return new_dictionary


# pylint: enable=broad-except


# pylint: disable=broad-except
def process_script_action():
    """
    Main processing.
    """
    try:
        total_coverage_dictionary = read_coverage_log_file("coverage.json")
        rules_coverage_dictionary = read_coverage_log_file("coverage.rules.json")
        database_coverage_dictionary = read_coverage_log_file("coverage.database.json")
        other_coverage_dictionary = read_coverage_log_file("coverage.other.json")

        coverage_dictionary = {}
        coverage_dictionary["total"] = total_coverage_dictionary
        coverage_dictionary["rules"] = rules_coverage_dictionary
        coverage_dictionary["database"] = database_coverage_dictionary
        coverage_dictionary["other"] = other_coverage_dictionary

        output_path = os.path.join(__BASE_DIR, "coverage-summary.json")
        with open(output_path, "w", encoding=__DEFAULT_FILE_ENCODING) as output_file:
            json.dump(coverage_dictionary, output_file, indent=4)
    except Exception as this_exception:
        print(f"Unable to generate combined coverage file: {this_exception}")
        return 1
    return 0


# pylint: enable=broad-except


sys.exit(process_script_action())
