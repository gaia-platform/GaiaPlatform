#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the push requirements for the GAIA section.
"""

from gdev.sections.gaia.build import GenGaiaBuild
from gdev.sections._abc.push import GenAbcPush


class GenGaiaPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the GAIA section.
    """

    @property
    def build(self) -> GenGaiaBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenGaiaBuild(self.options)
