#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the ENV section.
"""
from .build import GenEnvBuild
from .._abc.run import GenAbcRun


class GenEnvRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the ENV section.
    """

    @property
    def build(self) -> GenEnvBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenEnvBuild(self.options)
