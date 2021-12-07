#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to capture the output from the -m flag of suite.sh and translate it
into json.
"""

import sys
import json
import argparse


def process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Construct a Gaia configuration file from a JSON blob."
    )
    parser.add_argument(
        "--log-file",
        dest="log_file_name",
        required=True,
        action="store",
        help="Log file containing memory information written by scan_memory.py",
    )
    parser.add_argument(
        "--index-file",
        dest="index_file_name",
        required=True,
        action="store",
        help="Index file containing markers written by suite.sh",
    )
    parser.add_argument(
        "--output",
        dest="output_file_name",
        required=True,
        action="store",
        help="Output file name for the combined information.",
    )
    return parser.parse_args()


def process_script_action():
    """
    Process the posting of the message.
    """
    args = process_command_line()

    main_dictionary = {}

    with open(args.log_file_name) as input_file:
        sample_dictionary = {}
        main_dictionary["samples"] = sample_dictionary
        for next_line in input_file:
            next_line = next_line.strip()
            split_next_line = next_line.split(",")

            application_dictionary = {}
            memory_metrics = {}

            timestamp = int(split_next_line[0])
            application_name = split_next_line[5]
            if application_name in sample_dictionary:
                application_dictionary = sample_dictionary[application_name]
            else:
                sample_dictionary[application_name] = application_dictionary
            application_dictionary[timestamp] = memory_metrics

            memory_metrics["private"] = float(split_next_line[1])
            memory_metrics["shared"] = float(split_next_line[2])
            memory_metrics["total"] = float(split_next_line[3])

    with open(args.index_file_name) as input_file:
        marker_dictionary = {}
        main_dictionary["markers"] = marker_dictionary
        for next_line in input_file:
            next_line = next_line.strip()
            if next_line:
                split_next_line = next_line.split("-", maxsplit=1)

                timestamp = int(split_next_line[0])
                marker_dictionary[timestamp] = split_next_line[1].strip()

    with open(args.output_file_name, "wt") as write_file:
        json.dump(main_dictionary, write_file, indent=4)


sys.exit(process_script_action())
