#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the push requirements for the PIP section.
"""
from .build import GenPipBuild
from .._abc.push import GenAbcPush


class GenPipPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the PIP section.
    """

    @property
    def build(self) -> GenPipBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenPipBuild(self.options)
