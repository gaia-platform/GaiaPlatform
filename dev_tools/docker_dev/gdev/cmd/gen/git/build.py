#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the GIT section.
"""

from gdev.cmd.gen.git.dockerfile import GenGitDockerfile
from gdev.cmd.gen._abc.build import GenAbcBuild


class GenGitBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the GIT section.
    """

    @property
    def dockerfile(self) -> GenGitDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenGitDockerfile(self.options)
