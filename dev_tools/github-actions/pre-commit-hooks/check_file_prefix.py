#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to search for and remove any occurences of multiple empty lines.
"""

import argparse
import sys

__DEFAULT_FILE_ENCODING = "utf-8"


def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(description="Fill in.")
    parser.add_argument(
        "--prefix",
        dest="file_prefix",
        action="store",
        required=True,
        help="Prefix to scan for at the start of the file.",
    )
    parser.add_argument("filenames", nargs="*", help="Filenames to fix")
    return parser.parse_args()


def __check_file(filename, prefix_lines_array):

    with open(filename, mode="rt", encoding=__DEFAULT_FILE_ENCODING) as file_processed:
        lines = file_processed.readlines()

    did_all_lines_match = len(lines) >= len(prefix_lines_array)
    if did_all_lines_match:
        for prefix_line_index, prefix_line in enumerate(prefix_lines_array):
            input_line = lines[prefix_line_index][0:-1]
            did_all_lines_match = input_line == prefix_line
            if not did_all_lines_match:
                break
    return did_all_lines_match


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    return_code = 0

    prefix_lines_array = args.file_prefix.replace("\\n", "\n").split("\n")
    for filename in args.filenames:
        if not __check_file(filename, prefix_lines_array):
            print(f"Missing prefix: {filename}")
            return_code = 1

    if return_code:
        print("")
        print(
            "The files with the above file paths do not contain the specified prefix of:"
        )
        print("```")
        print("\n".join(prefix_lines_array))
        print("```")

    return return_code


sys.exit(process_script_action())
