#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the GIT section.
"""
from gdev.sections._abc.run import GenAbcRun
from gdev.sections.git.build import GenGitBuild


class GenGitRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the GIT section.
    """

    @property
    def build(self) -> GenGitBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenGitBuild(self.options)
