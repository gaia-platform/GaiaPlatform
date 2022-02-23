#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the WEB section.
"""
from .build import GenWebBuild
from .._abc.run import GenAbcRun


class GenWebRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the WEB section.
    """

    @property
    def build(self) -> GenWebBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenWebBuild(self.options)
