#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

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
    parser.add_argument("filenames", nargs="*", help="Filenames to fix")
    return parser.parse_args()


def __fix_file(filename):

    try:
        with open(
            filename, mode="rt", encoding=__DEFAULT_FILE_ENCODING
        ) as file_processed:
            lines = file_processed.readlines()
        newlines = lines[:]
        line_index = 0
        last_line_was_blank = False
        while line_index < len(newlines):
            this_line_is_blank = newlines[line_index] == "\n"
            if this_line_is_blank and last_line_was_blank:
                del newlines[line_index]
            else:
                last_line_was_blank = this_line_is_blank
                line_index += 1
        if newlines != lines:
            with open(
                filename, mode="wt", encoding=__DEFAULT_FILE_ENCODING
            ) as file_processed:
                for line in newlines:
                    file_processed.write(line)
            return True
    except UnicodeDecodeError:
        print(f"Skipping non-text file '{filename}'.")
    return False


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    return_code = 0
    for filename in args.filenames:
        if __fix_file(filename):
            print(f"Fixing {filename}")
            return_code = 1

    return return_code


sys.exit(process_script_action())
