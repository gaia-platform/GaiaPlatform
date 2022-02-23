#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `cfg` subcommand entry point.
"""
from gdev.third_party.atools import memoize
from .gen.run.cfg import GenRunCfg


class Cfg(GenRunCfg):
    """
    Class to provide for the `cfg` subcommand entry point.
    """

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        print(
            '\n'.join(
                self.get_lines(
                    cfg_enables=self.options.cfg_enables,
                    path=self.path
                )
            )
        )
