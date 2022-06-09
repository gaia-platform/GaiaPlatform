#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to satisfy the push requirements for the RUN section.
"""

from gdev.sections.run.build import GenRunBuild
from gdev.sections._abc.push import GenAbcPush


class GenRunPush(GenAbcPush):
    """
    Class to satisfy the push requirements for the RUN section.
    """

    @property
    def build(self) -> GenRunBuild:
        """
        Return the class that will be used to generate the build requirements.
        """
        return GenRunBuild(self.options)
