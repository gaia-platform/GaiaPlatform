#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the push requirements for the ENV section.
"""

from gdev.cmd.gen.env.build import GenEnvBuild
from gdev.cmd.gen._abc.push import GenAbcPush


class GenEnvPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the ENV section.
    """

    @property
    def build(self) -> GenEnvBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenEnvBuild(self.options)
