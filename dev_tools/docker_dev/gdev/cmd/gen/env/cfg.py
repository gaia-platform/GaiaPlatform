#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide a subclass of the GenAbcCfg class for the Env section.
"""

from gdev.cmd.gen._abc.cfg import GenAbcCfg


class GenEnvCfg(GenAbcCfg):
    """
    Class to provide a subclass of the GenAbcCfg class for the Env section.

    Note that this class is empty, but kept to maintain a subcommand structure
    that is equivalent to the other subcommands, such as build.
    """
