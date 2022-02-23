#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the PIP section.
"""
from .build import GenPipBuild
from .._abc.run import GenAbcRun


class GenPipRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the PIP section.
    """

    @property
    def build(self) -> GenPipBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenPipBuild(self.options)
