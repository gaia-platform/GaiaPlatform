#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Simple module to copy only those lines that occur before a given line.
"""

import sys
import argparse
import pathlib

__DEFAULT_FILE_ENCODING = "utf-8"


def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Copy a file from one place to another after we hit a specified line."
    )
    parser.add_argument(
        "--input-file",
        dest="input_path",
        action="store",
        required=True,
        help="File to use as input.",
    )
    parser.add_argument(
        "--output-file",
        dest="output_path",
        action="store",
        required=True,
        help="File to use as output.",
    )
    parser.add_argument(
        "--start-line",
        dest="start_line",
        action="store",
        required=True,
        help="Contents of line to start copying at.",
    )
    return parser.parse_args()


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    lines_in_text_file = (
        pathlib.Path(args.input_path)
        .read_text(encoding=__DEFAULT_FILE_ENCODING)
        .split("\n")
    )
    have_seen_start_line = False
    with pathlib.Path(args.output_path).open(
        mode="w", encoding=__DEFAULT_FILE_ENCODING
    ) as output_file:
        for next_line in lines_in_text_file:
            if not have_seen_start_line:
                have_seen_start_line = next_line == args.start_line
            if have_seen_start_line:
                output_file.write(next_line + "\n")


sys.exit(process_script_action())
