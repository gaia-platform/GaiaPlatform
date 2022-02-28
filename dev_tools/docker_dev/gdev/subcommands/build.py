#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `cfg` subcommand entry point.
"""

from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from gdev.sections.run.build import GenRunBuild


class Build(Dependency):
    """
    Class to provide for the `build` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Build the image based on the assembled dockerfile."

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunBuild(self.options).cli_entrypoint()
