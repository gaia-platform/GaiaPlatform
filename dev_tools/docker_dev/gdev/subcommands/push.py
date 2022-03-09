#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `push` subcommand entry point.
"""

import sys
from gdev.options import Options
from gdev.command_line import CommandLine
from gdev.sections.run.push import GenRunPush


class Push:
    """
    Class to provide for the `push` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Build the image, if required, and push the image to the image registry."

    @classmethod
    def cli_entrypoint(cls, options: Options) -> None:
        """
        Execution entrypoint for this module.
        """
        if (
            not options.registry
            and not CommandLine.is_backward_compatibility_mode_enabled()
        ):
            print(
                "Error: The --registry arguments must specify the registry to push to.",
                file=sys.stderr,
            )
            sys.exit(1)
        GenRunPush(options).run()
