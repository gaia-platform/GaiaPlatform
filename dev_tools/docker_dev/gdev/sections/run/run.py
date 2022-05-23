#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to satisfy the run requirements for the RUN section.
"""

from gdev.sections.run.build import GenRunBuild
from gdev.sections._abc.run import GenAbcRun


class GenRunRun(GenAbcRun):
    """
    Class to satisfy the run requirements for the RUN section.
    """

    @property
    def build(self) -> GenRunBuild:
        """
        Return the class that will be used to generate the run requirements.
        """
        return GenRunBuild(self.options)
