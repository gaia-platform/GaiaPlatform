#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the push requirements for the APT section.
"""

from gdev.cmd.gen.apt.build import GenAptBuild
from gdev.cmd.gen._abc.push import GenAbcPush


class GenAptPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the APT section.
    """

    @property
    def build(self) -> GenAptBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenAptBuild(self.options)
