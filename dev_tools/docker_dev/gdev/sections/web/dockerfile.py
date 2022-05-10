#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to generate the WEB section of the dockerfile.
"""

from gdev.third_party.atools import memoize
from gdev.sections._abc.dockerfile import GenAbcDockerfile
from gdev.sections.web.cfg import GenWebCfg


class GenWebDockerfile(GenAbcDockerfile):
    """
    Class to generate the WEB section of the dockerfile.
    """

    @property
    def cfg(self) -> GenWebCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenWebCfg(self.options)

    @memoize
    def get_from_section(self) -> str:
        """
        Return text for the FROM line of the final build stage.
        """
        from_section = f"FROM web_base AS {self.get_name()}"

        self.log.debug("from_section = %s", from_section)

        return from_section

    @memoize
    def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """

        if section_lines := self.cfg.get_section_lines():
            formatted_section_lines = " \\\n        ".join(section_lines)
            run_statement = (
                "RUN wget "
                + formatted_section_lines
                + " \\\n    && apt-get remove --autoremove -y wget"
            )
        else:
            run_statement = ""

        self.log.debug("run_statement = %s", run_statement)

        return run_statement
