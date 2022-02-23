#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to execute the steps necessary to generate a dockerfile based
on the configuration.
"""

from typing import Iterable

from gdev.third_party.atools import memoize
from gdev.cmd.gen._abc.dockerfile import GenAbcDockerfile
from gdev.cmd.gen.run.cfg import GenRunCfg


class GenRunDockerfile(GenAbcDockerfile):
    """
    Class to execute the steps necessary to generate a dockerfile based
    on the configuration.
    """

    @property
    def cfg(self) -> GenRunCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenRunCfg(self.options)

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_env_section(self) -> str:
        """
        Return text for the ENV section of the final build stage.
        """
        from gdev.cmd.gen.pre_run.dockerfile import GenPreRunDockerfile

        env_section = GenPreRunDockerfile(self.options).get_env_section()

        self.log.debug("env_section = %s", env_section)

        return env_section

    # pylint: enable=import-outside-toplevel

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """
        from gdev.cmd.gen.pre_run.dockerfile import GenPreRunDockerfile

        input_dockerfiles = tuple([GenPreRunDockerfile(self.options)])

        self.log.debug("input_dockerfiles = %s", input_dockerfiles)

        return input_dockerfiles

    # pylint: enable=import-outside-toplevel

    @memoize
    def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """
        if section_lines := self.cfg.get_section_lines():
            run_section = "RUN " + " \\\n    && ".join(section_lines)
        else:
            run_section = ""

        self.log.debug("run_section = %s", run_section)

        return run_section
