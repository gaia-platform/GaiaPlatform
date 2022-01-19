#! /usr/bin/python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to replace the version information in the `gaia_version.hpp.in` file with
the information provided by GitHub when executing the actions.
"""

import argparse
import sys
import pathlib
import os
import re

__VERSION_HPP_PATH = "production/inc/gaia_internal/common/gaia_version.hpp.in"
__REPLACE_BOUNDARY_CHARACTER = "@"


def __process_command_line():
    """
    Process the arguments on the command line.
    """
    parser = argparse.ArgumentParser(
        description="Perform actions based on a graph of GDEV configuration files."
    )
    parser.add_argument(
        "--directory",
        dest="configuration_directory",
        action="store",
        required=True,
        help="Directory to root the configuration search in.",
    )
    parser.add_argument(
        "--version",
        dest="gaia_version",
        action="store",
        required=True,
        help="Version of Gaia that is being built.",
    )
    parser.add_argument(
        "--pre-release",
        dest="gaia_pre_release",
        action="store",
        help="Prerelease tag associated with the Gaia that is being built.",
    )
    parser.add_argument(
        "--build-number",
        dest="build_number",
        action="store",
        required=True,
        help="Build number associated with the build.",
    )
    parser.add_argument(
        "--stdout",
        dest="output_to_stdout",
        action="store_true",
        help="Output the results to stdout instead of modifying the original file.",
    )
    return parser.parse_args()


def __munge_individual_line(line_to_parse, replacement_map, output_file_list):
    """
    Given a line read in from the file, replace any strings that are enclosed within
    a pair of __REPLACE_BOUNDARY_CHARACTER (@) characters.
    """

    start_index = line_to_parse.find(__REPLACE_BOUNDARY_CHARACTER)
    if start_index != -1:
        end_index = line_to_parse.find(__REPLACE_BOUNDARY_CHARACTER, start_index + 1)
        if end_index != -1:
            middle_part = line_to_parse[start_index : end_index + 1]
            if middle_part not in replacement_map:
                print(f"Replacement text '{middle_part}' not found.")
                sys.exit(1)
            new_middle_part = replacement_map[middle_part]
            line_to_parse = (
                line_to_parse[:start_index]
                + new_middle_part
                + line_to_parse[end_index + 1 :]
            )

    output_file_list.append(line_to_parse)


def __create_replacement_map(args):
    """
    Given a list of all the replacement values, figure out the values and prime the
    replacement map with them.
    """

    gaia_version_regex = r"^([0-9]+)\.([0-9]+)\.([0-9]+)$"
    match_result = re.match(gaia_version_regex, args.gaia_version)
    if not match_result:
        print(
            f"Supplied version number '{args.gaia_version}' is not a valid 3-part version number."
        )
        return 1

    replacement_map = {}
    replacement_map["@production_VERSION_MAJOR@"] = match_result.groups()[0]
    replacement_map["@production_VERSION_MINOR@"] = match_result.groups()[1]
    replacement_map["@production_VERSION_PATCH@"] = match_result.groups()[2]

    replacement_map["@PRE_RELEASE_IDENTIFIER@"] = (
        args.gaia_pre_release if args.gaia_pre_release else ""
    )

    replacement_map["@BUILD_NUMBER@"] = args.build_number

    replacement_map["@GIT_HEAD_HASH@"] = ""
    replacement_map["@GIT_MASTER_HASH@"] = ""

    replacement_map["@IS_CI_BUILD@"] = "true"
    return replacement_map


def process_script_action():
    """
    Process the posting of the message.
    """
    args = __process_command_line()

    # Make sure that we have a valid directory.
    if not os.path.exists(args.configuration_directory):
        print(
            f"Specified root directory {args.configuration_directory} does not exist."
        )
        return 1
    if not os.path.isdir(args.configuration_directory):
        print(
            f"Specified root directory {args.configuration_directory} is not a directory."
        )
        return 1

    replacement_map = __create_replacement_map(args)

    hpp_path = pathlib.Path(args.configuration_directory) / __VERSION_HPP_PATH
    lines_in_text_file = hpp_path.read_text().split("\n")

    output_lines = []
    for next_line in lines_in_text_file:
        __munge_individual_line(next_line, replacement_map, output_lines)

    if args.output_to_stdout:
        print("\n".join(output_lines))
    else:
        hpp_path.write_text("\n".join(output_lines))
    return 0


sys.exit(process_script_action())
