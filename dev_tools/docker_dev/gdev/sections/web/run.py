#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the WEB section.
"""

from gdev.sections.web.build import GenWebBuild
from gdev.sections._abc.run import GenAbcRun


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
