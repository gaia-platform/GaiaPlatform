#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `cfg` subcommand entry point.
"""

from gdev.third_party.atools import memoize
from gdev.sections.run.cfg import GenRunCfg


class Cfg(GenRunCfg):
    """
    Class to provide for the `cfg` subcommand entry point.
    """

    @classmethod
    def cli_entrypoint_description(cls):
        """
        Description to display in help for this subcommand.
        """
        return "Generate the configuration used as the basis for the dockerfile."

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunCfg(self.options).cli_entrypoint()
