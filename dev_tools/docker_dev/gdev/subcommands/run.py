#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `run` subcommand entry point.
"""

from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from gdev.sections.run.run import GenRunRun


class Run(Dependency):
    """
    Class to provide for the `run` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Build the image, if required, and execute a container for GaiaPlatform development."

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunRun(self.options).cli_entrypoint()
