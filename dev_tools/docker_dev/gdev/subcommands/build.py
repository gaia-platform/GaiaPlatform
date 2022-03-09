#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `cfg` subcommand entry point.
"""

from gdev.sections.run.build import GenRunBuild
from gdev.sections._custom.build import GenCustomBuild


class Build:
    """
    Class to provide for the `build` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Build the image based on the assembled dockerfile."

    @classmethod
    def cli_entrypoint(cls, options) -> None:
        """
        Execution entrypoint for this module.
        """
        build = GenRunBuild(options)
        if options.mixins:
            build = GenCustomBuild(options=options, base_build=build)

        build.run()
