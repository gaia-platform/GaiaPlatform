#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide a single entry point from the operating system.
"""

from __future__ import annotations

import sys
import logging
from importlib import import_module
from typing import Sequence
from gdev.sections._abc.gdev_action import GdevAction
from gdev.custom.gaia_path import GaiaPath
from gdev.options import Options
from gdev.third_party.argcomplete import autocomplete, FilesCompleter
from gdev.parser_structure import ParserStructure
from gdev.command_line import CommandLine
from gdev.sections.section_action_exception import SectionActionException


# pylint: disable=too-few-public-methods
class DockerDev:
    """
    Class to provide a single entry point from the operating system.
    """

    @staticmethod
    def __of_args(args: Sequence[str]) -> GdevAction:
        """
        Return Dependency constructed by parsing args as if from sys.argv.
        """

        # Generate the parser structure, and the command line arguments based off of that.
        #
        # Note that while `is_backward_compatible_guess` is named as a guess, it is
        # a good guess.  Ideally, the parser would return this value and the options
        # would trigger off that parsing of the argument.  However, since there are
        # arguments and attributes of those arguments that have changed since the
        # original GDEV, we need to guess at whether the application is in its backward
        # compatible mode BEFORE the argument parsing.
        current_args = (
            args[: args.index(CommandLine.DOUBLE_DASH_ARGUMENT)]
            if CommandLine.DOUBLE_DASH_ARGUMENT in args
            else args[:]
        )
        is_backward_compatible_guess = "--backward" in current_args
        parser = CommandLine.get_parser(
            ParserStructure.of_command_parts(tuple()), is_backward_compatible_guess
        )

        # Setup autocomplete.
        autocomplete(
            parser, default_completer=FilesCompleter(allowednames="", directories=False)
        )

        # Do the actual parsing of the arguments.  If we have nothing on the command line,
        # make sure to show the default help command. Otherwise, intrepret the arguments
        # inline.
        parsed_args = parser.parse_args(args).__dict__
        if not parsed_args:
            parser.parse_args([*args, "--help"])
            sys.exit(1)
        CommandLine.interpret_arguments(parsed_args)

        # Set the options up.  The two command_* elements are not part of the Options
        # class, so pop them before the Options class is initialized.
        command_class = parsed_args.pop("command_class")
        command_module = parsed_args.pop("command_module")
        options = Options(
            target=f"{GaiaPath.cwd().relative_to(GaiaPath.repo())}", **parsed_args
        )

        # Create the subcommand that was indicated, including all options.
        return getattr(import_module(command_module), command_class)(), options

    @staticmethod
    def main():
        """
        Main entry point from the operating system.
        """
        try:
            subcommand, options = DockerDev.__of_args(tuple(sys.argv[1:]))

            logging.basicConfig(level=options.log_level)

            try:
                subcommand.cli_entrypoint(options)
            except SectionActionException as this_exception:
                print(f"\n{this_exception}", file=sys.stderr)
            finally:
                logging.shutdown()
        except ValueError as this_exception:
            print(f"\n{this_exception}", file=sys.stderr)

        return 0


# pylint: enable=too-few-public-methods

if __name__ == "__main__":
    DockerDev().main()
