#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the PIP section.
"""

from gdev.cmd.gen.pip.dockerfile import GenPipDockerfile
from gdev.cmd.gen._abc.build import GenAbcBuild


class GenPipBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the PIP section.
    """

    @property
    def dockerfile(self) -> GenPipDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenPipDockerfile(self.options)
