#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide for the `push` subcommand entry point.
"""
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.push import GenRunPush


class Push(Dependency):
    """
    Class to provide for the `push` subcommand entry point.
    """

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunPush(self.options).cli_entrypoint()
