#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the push requirements for the WEB section.
"""
from .build import GenWebBuild
from .._abc.push import GenAbcPush


class GenWebPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the WEB section.
    """

    @property
    def build(self) -> GenWebBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenWebBuild(self.options)
