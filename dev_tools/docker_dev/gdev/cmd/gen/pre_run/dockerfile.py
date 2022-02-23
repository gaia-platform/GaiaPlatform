#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to generate the PRERUN section of the dockerfile.
"""

from typing import Iterable

from gdev.third_party.atools import memoize
from gdev.cmd.gen._abc.dockerfile import GenAbcDockerfile
from gdev.cmd.gen.pre_run.cfg import GenPreRunCfg


class GenPreRunDockerfile(GenAbcDockerfile):
    """
    Class to generate the PRERUN section of the dockerfile.
    """

    @property
    def cfg(self) -> GenPreRunCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenPreRunCfg(self.options)

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_env_section(self) -> str:
        """
        Return text for the ENV section of the final build stage.
        """
        from gdev.cmd.gen.env.dockerfile import GenEnvDockerfile

        seen_env_dockerfiles = set()

        def inner(env_dockerfile: GenEnvDockerfile) -> Iterable[str]:
            env_section_parts = []
            if env_dockerfile not in seen_env_dockerfiles:
                seen_env_dockerfiles.add(env_dockerfile)
                for input_env_dockerfile in env_dockerfile.get_input_dockerfiles():
                    env_section_parts += inner(
                        GenEnvDockerfile(input_env_dockerfile.options)
                    )
                if section_lines := env_dockerfile.cfg.get_section_lines():
                    env_section_parts.append(f"# {env_dockerfile}")
                    for line in section_lines:
                        env_section_parts.append(f"ENV {line}")
            return env_section_parts

        env_section = "\n".join(inner(GenEnvDockerfile(self.options)))

        self.log.debug("env_section = %s", env_section)

        return env_section

    # pylint: enable=import-outside-toplevel

    # pylint: enable=import-outside-toplevel

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """
        from gdev.cmd.gen.env.dockerfile import GenEnvDockerfile
        from gdev.cmd.gen.apt.dockerfile import GenAptDockerfile
        from gdev.cmd.gen.gaia.dockerfile import GenGaiaDockerfile
        from gdev.cmd.gen.git.dockerfile import GenGitDockerfile
        from gdev.cmd.gen.pip.dockerfile import GenPipDockerfile
        from gdev.cmd.gen.web.dockerfile import GenWebDockerfile

        input_dockerfiles = tuple(
            [
                GenAptDockerfile(self.options),
                GenEnvDockerfile(self.options),
                GenGaiaDockerfile(self.options),
                GenGitDockerfile(self.options),
                GenPipDockerfile(self.options),
                GenWebDockerfile(self.options),
            ]
        )

        self.log.debug("input_dockerfiles = %s", input_dockerfiles)

        return input_dockerfiles

    # pylint: enable=import-outside-toplevel

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
