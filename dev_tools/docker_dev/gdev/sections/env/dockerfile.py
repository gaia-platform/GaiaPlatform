#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to generate the ENV section of the dockerfile.
"""

from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from gdev.sections._abc.dockerfile import GenAbcDockerfile
from gdev.sections.env.cfg import GenEnvCfg


class GenEnvDockerfile(GenAbcDockerfile):
    """
    Class to generate the ENV section of the dockerfile.
    """

    @property
    def cfg(self) -> GenEnvCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenEnvCfg(self.options)

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """
        from gdev.sections.gaia.dockerfile import GenGaiaDockerfile

        input_dockerfiles = []
        for section_line in GenGaiaDockerfile(self.options).cfg.get_section_lines():
            input_dockerfiles.append(
                GenEnvDockerfile(replace(self.options, target=section_line))
            )
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug("input_dockerfiles = %s", input_dockerfiles)

        return input_dockerfiles

    # pylint: enable=import-outside-toplevel
