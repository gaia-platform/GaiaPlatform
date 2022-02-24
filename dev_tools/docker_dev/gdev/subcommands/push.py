#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `push` subcommand entry point.
"""

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
    def cli_entrypoint(cls, options) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunPush(options).run()
