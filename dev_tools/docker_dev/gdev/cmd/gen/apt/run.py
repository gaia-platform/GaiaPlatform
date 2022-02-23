#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the APT section.
"""

from .build import GenAptBuild
from .._abc.run import GenAbcRun


class GenAptRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the APT section.
    """

    @property
    def build(self) -> GenAptBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenAptBuild(self.options)
