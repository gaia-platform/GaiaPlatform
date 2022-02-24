#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the push requirements for the PRERUN section.
"""

from gdev.sections.pre_run.build import GenPreRunBuild
from gdev.sections._abc.push import GenAbcPush


class GenPreRunPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the PRERUN section.
    """

    @property
    def build(self) -> GenPreRunBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenPreRunBuild(self.options)
