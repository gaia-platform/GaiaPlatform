#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the APT section.
"""

from gdev.sections.apt.dockerfile import GenAptDockerfile
from gdev.sections._abc.build import GenAbcBuild


class GenAptBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the APT section.
    """

    @property
    def dockerfile(self) -> GenAptDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenAptDockerfile(self.options)
