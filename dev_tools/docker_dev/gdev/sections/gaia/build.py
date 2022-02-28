#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the GAIA section.
"""

from gdev.sections.gaia.dockerfile import GenGaiaDockerfile
from gdev.sections._abc.build import GenAbcBuild


class GenGaiaBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the GAIA section.
    """

    @property
    def dockerfile(self) -> GenGaiaDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenGaiaDockerfile(self.options)
