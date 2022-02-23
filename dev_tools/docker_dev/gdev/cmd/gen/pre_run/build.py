#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to satisfy the build requirements to generate the dockerfile for the PRERUN section.
"""
from .dockerfile import GenPreRunDockerfile
from .._abc.build import GenAbcBuild


class GenPreRunBuild(GenAbcBuild):
    """
    Class to satisfy the build requirements to generate the dockerfile for the PRERUN section.
    """

    @property
    def dockerfile(self) -> GenPreRunDockerfile:
        """
        Return the class that will be used to generate its portion of the dockerfile.
        """
        return GenPreRunDockerfile(self.options)
