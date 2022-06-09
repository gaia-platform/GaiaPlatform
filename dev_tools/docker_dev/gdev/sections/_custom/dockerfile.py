#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to generate the RUN section of the dockerfile where MIXINs are used.
"""

from dataclasses import dataclass, replace
import os
from textwrap import dedent
from typing import Iterable

from gdev.custom.gaia_path import GaiaPath
from gdev.third_party.atools import memoize
from gdev.sections._abc.dockerfile import GenAbcDockerfile
from gdev.sections._custom.cfg import GenCustomCfg


@dataclass(frozen=True, repr=False)
class GenCustomDockerfile(GenAbcDockerfile):
    """
    Class to provide a customized GenAbcDockerfile where user properties
    from the host of the container are taken into account.
    """

    base_dockerfile: GenAbcDockerfile

    @property
    def cfg(self) -> GenCustomCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        return GenCustomCfg(self.options)

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_env_section(self) -> str:
        """
        Return text for the ENV section of the final build stage.
        """
        from gdev.sections.pre_run.dockerfile import GenPreRunDockerfile

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
        from gdev.sections.run.dockerfile import GenRunDockerfile

        input_dockerfiles = [self.base_dockerfile]
        for line in self.cfg.get_mixin_lines():
            input_dockerfiles.append(
                GenRunDockerfile(replace(self.options, target=line))
            )
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug("input_dockerfiles = %s", input_dockerfiles)

        return input_dockerfiles

    # pylint: enable=import-outside-toplevel

    @memoize
    def get_text(self) -> str:
        """
        Return the full text for this dockerfile, including all build stages.

        Note that since these mixins interact directly with the host from within the
        container, these commands are run to ensure that the user information on the
        host is used to set permissions and ownership.
        """
        text_parts = [super().get_text()]

        if {"clion", "sudo", "vscode"} & self.options.mixins:
            uid = os.getuid()
            gid = os.getgid()
            home = GaiaPath.home()
            login = os.getlogin()
            text_parts.append(
                dedent(
                    rf"""
                RUN groupadd -r -o -g {gid} {login} \
                    && useradd {login} -l -r -o -u {uid} -g {gid} -G sudo \
                    && mkdir -p {home} \
                    && chown {login}:{login} {home} \
                    && echo "{login} ALL=(ALL:ALL) NOPASSWD: ALL" >> /etc/sudoers \
                    && touch {home}/.sudo_as_admin_successful \
                    && chown -R {login}:{login} {GaiaPath.repo().image_build()}
            """
                ).strip()
            )

        text = "\n".join(text_parts)

        self.log.debug("text = %s", text)

        return text
