#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to handle the processing of the 'gaia' section of the 'gdev.cfg' file.
"""

from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from gdev.sections._abc.dockerfile import GenAbcDockerfile
from gdev.sections.gaia.cfg import GenGaiaCfg


class GenGaiaDockerfile(GenAbcDockerfile):
    """
    Class to handle the processing of the 'gaia' section of the 'gdev.cfg' file.
    """

    @property
    def cfg(self) -> GenGaiaCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenGaiaCfg(self.options)

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """
        from gdev.sections.run.dockerfile import GenRunDockerfile

        input_dockerfiles = []
        for section_line in self.cfg.get_section_lines():
            input_dockerfiles.append(
                GenRunDockerfile(replace(self.options, target=section_line))
            )
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug("input_dockerfiles = %s", input_dockerfiles)

        return input_dockerfiles

    # pylint: enable=import-outside-toplevel
