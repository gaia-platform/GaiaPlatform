#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide a single entry point from the operating system.
"""

from __future__ import annotations

import sys
import logging
from importlib import import_module
from typing import Sequence
from gdev.dependency import Dependency
from gdev.custom.gaia_path import GaiaPath
from gdev.options import Options
from gdev.third_party.argcomplete import autocomplete, FilesCompleter
from gdev.parser_structure import ParserStructure
from gdev.command_line import CommandLine


# pylint: disable=too-few-public-methods
class DockerDev:
    """
    Class to provide a single entry point from the operating system.
    """

    @staticmethod
    def __of_args(args: Sequence[str]) -> Dependency:
        """
        Return Dependency constructed by parsing args as if from sys.argv.
        """

        # Generate the parser structure, and the command line arguments based off of that.
        is_backward_compatible_guess = "--backward" in args
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
        return getattr(import_module(command_module), command_class)(options)

    @staticmethod
    def main():
        """
        Main entry point from the operating system.
        """
        dependency = DockerDev.__of_args(tuple(sys.argv[1:]))

        logging.basicConfig(level=dependency.options.log_level)

        try:
            dependency.cli_entrypoint()
        except dependency.Exception as this_exception:
            print(f"\n{this_exception}", file=sys.stderr)
        finally:
            logging.shutdown()

        return 0


# pylint: enable=too-few-public-methods

if __name__ == "__main__":
    DockerDev().main()
