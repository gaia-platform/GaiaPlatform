#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the run requirements for the GAIA section.
"""
from .build import GenGaiaBuild
from .._abc.run import GenAbcRun


class GenGaiaRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the GAIA section.
    """

    @property
    def build(self) -> GenGaiaBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenGaiaBuild(self.options)
