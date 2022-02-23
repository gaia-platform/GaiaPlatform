#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the ENV section.
"""

from gdev.cmd.gen.env.dockerfile import GenEnvDockerfile
from gdev.cmd.gen._abc.build import GenAbcBuild


class GenEnvBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the ENV section.
    """

    @property
    def dockerfile(self) -> GenEnvDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenEnvDockerfile(self.options)
