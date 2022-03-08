#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the RUN section.
"""

from gdev.sections.run.build import GenRunBuild
from gdev.sections._abc.run import GenAbcRun


class GenRunRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the RUN section.
    """

    @property
    def build(self) -> GenRunBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenRunBuild(self.options)
