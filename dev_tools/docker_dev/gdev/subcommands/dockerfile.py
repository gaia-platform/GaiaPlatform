#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `dockerfile` subcommand entry point.
"""

from gdev.command_line import CommandLine
from gdev.sections.run.dockerfile import GenRunDockerfile
from gdev.sections._custom.dockerfile import GenCustomDockerfile


class Dockerfile:
    """
    Class to provide for the `dockerfile` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Assemble the dockerfile based on the generated configuration."

    @classmethod
    def cli_entrypoint(cls, options) -> None:
        """
        Entry point for the command line.
        """
        dockerfile = GenRunDockerfile(options)
        if options.mixins:
            dockerfile = GenCustomDockerfile(
                options=options, base_dockerfile=dockerfile
            )
        dockerfile.run()
        if CommandLine.is_backward_compatibility_mode_enabled():
            print(dockerfile.path.read_text())
        else:
            print(f"Dockerfile written to: {dockerfile.path}")
