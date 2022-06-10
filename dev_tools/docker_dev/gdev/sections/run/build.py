#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to satisfy the build requirements to generate the dockerfile for the RUN section.
"""

from gdev.sections.run.dockerfile import GenRunDockerfile
from gdev.sections._abc.build import GenAbcBuild


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
