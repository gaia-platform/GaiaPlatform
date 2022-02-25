#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the RUN section.
"""

from gdev.cmd.gen.run.dockerfile import GenRunDockerfile
from gdev.cmd.gen._abc.build import GenAbcBuild


class GenRunBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the RUN section.
    """

    @property
    def dockerfile(self) -> GenRunDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenRunDockerfile(self.options)
