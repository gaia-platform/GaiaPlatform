#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the GIT section.
"""
from .._abc.build import GenAbcBuild
from .._abc.run import GenAbcRun


class GenGitRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the GIT section.
    """

    @property
    def build(self) -> GenAbcBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        from .build import GenGitBuild
        return GenGitBuild(self.options)
