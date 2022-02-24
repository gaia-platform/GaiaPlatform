#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the WEB section.
"""

from gdev.cmd.gen.web.dockerfile import GenWebDockerfile
from gdev.cmd.gen._abc.build import GenAbcBuild


class GenWebBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the WEB section.
    """

    @property
    def dockerfile(self) -> GenWebDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenWebDockerfile(self.options)
