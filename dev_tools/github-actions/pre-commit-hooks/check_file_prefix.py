#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to search for and remove any occurences of multiple empty lines.
"""

import argparse
import sys
import os

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


def __check_file(filename):

    with open(filename, mode="rt", encoding=__DEFAULT_FILE_ENCODING) as file_processed:
        lines = file_processed.readlines()
    return True


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    return_code = 0

    fg = args.file_prefix.replace("\\n", "\n")
    print(str(fg))

    for filename in args.filenames:
        if __check_file(filename):
            print(f"Fixing {filename}")
            return_code = 1

    return return_code


sys.exit(process_script_action())
