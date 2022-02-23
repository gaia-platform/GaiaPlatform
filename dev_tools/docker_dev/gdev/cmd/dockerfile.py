#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `dockerfile` subcommand entry point.
"""

from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from gdev.cmd.gen.run.dockerfile import GenRunDockerfile


class Dockerfile(Dependency):
    """
    Class to provide for the `dockerfile` subcommand entry point.
    """

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Entry point for the command line.
        """
        GenRunDockerfile(self.options).cli_entrypoint()
