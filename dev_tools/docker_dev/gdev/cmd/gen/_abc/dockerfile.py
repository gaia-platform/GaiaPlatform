#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to create a Dockerfile from the rules in the `gdev.cfg` file in the
current working directory.
"""

from __future__ import annotations
from abc import ABC, abstractmethod
from textwrap import dedent
from typing import Iterable

from gdev.custom.gaia_path import GaiaPath
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from gdev.cmd.gen._abc.cfg import GenAbcCfg


class GenAbcDockerfile(Dependency, ABC):
    """
    Class to create a Dockerfile from the rules in the `gdev.cfg` file in the current
    working directory.
    """

    @property
    @abstractmethod
    def cfg(self) -> GenAbcCfg:
        """
        Get the configuration associated with this portion of the dockerfile.
        """
        raise NotImplementedError

    @property
    def path(self) -> GaiaPath:
        """
        Return path where dockerfile is to be written.
        """
        path = (
            GaiaPath.repo()
            / ".gdev"
            / self.options.target
            / f"{self.cfg.section_name}.dockerfile.gdev"
        )

        self.log.debug("path = %s", path)

        return path

    @memoize
    def get_base_stages_text(self) -> str:
        """
        Get the text that applies to the base stages of the dockerfile.
        """

        base_stages_text = dedent(
            fr"""
            #syntax=docker/dockerfile-upstream:master-experimental

            # Static definition of base stages.
            FROM {self.options.base_image} AS base
            RUN groupadd -r -g 101 messagebus \
                && groupadd -r -g 102 postgres \
                && groupadd -r -g 103 ssh \
                && groupadd -r -g 104 ssl-cert \
                && groupadd -r -g 105 systemd-timesync \
                && groupadd -r -g 106 systemd-journal \
                && groupadd -r -g 107 systemd-network \
                && groupadd -r -g 108 systemd-resolve \
                && useradd messagebus -l -r -u 101 -g 101 \
                && useradd postgres -l -r -u 102 -g 102 -G ssl-cert \
                && useradd systemd-timesync -l -r -u 103 -g 105 -d /run/systemd \
                    -s /usr/sbin/nologin \
                && useradd systemd-network -l -r -u 104 -g 107 -d /run/systemd \
                    -s /usr/sbin/nologin \
                && useradd systemd-resolve -l -r -u 105 -g 108 -d /run/systemd \
                    -s /usr/sbin/nologin \
                && useradd sshd -l -r -u 106 -d /run/sshd -s /usr/sbin/nologin

            FROM base AS apt_base
            RUN echo "APT::Acquire::Retries \"5\";" > /etc/apt/apt.conf.d/80-retries \
                && apt-get update

            FROM apt_base AS git_base
            RUN apt-get update \
                && DEBIAN_FRONTEND=noninteractive apt-get install -y git \
                && apt-get clean

            FROM apt_base AS pip_base
            RUN apt-get update \
                && DEBIAN_FRONTEND=noninteractive apt-get install -y python3-pip \
                && apt-get clean

            FROM apt_base AS web_base
            RUN apt-get update \
                && DEBIAN_FRONTEND=noninteractive apt-get install -y wget \
                && apt-get clean
        """
        ).strip()

        self.log.debug("base_stages_text = %s", base_stages_text)

        return base_stages_text

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def get_copy_section(self) -> str:
        """
        Return text for the COPY section of the final build stage.
        """
        from gdev.cmd.gen.pre_run.dockerfile import GenPreRunDockerfile

        seen_dockerfiles = set()

        # Calculating which build stages to copy from has a tricky problem problems. Docker
        # sometimes fails to build images with inline caching if there is no RUN command in the
        # build stage, so we cannot have such stages. In cases where such a stage would be an input
        # to the current stage, we must instead recursively select the empty stage's non-input
        # stages instead.
        def inner(dockerfile: GenAbcDockerfile) -> Iterable[str]:
            copy_section_parts = []
            if dockerfile not in seen_dockerfiles:
                seen_dockerfiles.add(dockerfile)
                for input_dockerfile in dockerfile.get_input_dockerfiles():
                    if input_dockerfile.get_run_section():
                        copy_section_parts.append(
                            f"COPY --from={input_dockerfile.get_name()} / /"
                        )
                    else:
                        copy_section_parts += inner(input_dockerfile)

                path = dockerfile.cfg.path.parent
                if isinstance(dockerfile, GenPreRunDockerfile) and set(
                    path.iterdir()
                ) - {dockerfile.cfg.path}:
                    copy_section_parts.append(
                        f"COPY {path.context()} {path.image_source()}"
                    )

            return copy_section_parts

        copy_section = "\n".join(inner(self))

        self.log.debug("copy_section = %s", copy_section)

        return copy_section

    # pylint: enable=import-outside-toplevel

    @memoize
    def get_env_section(self) -> str:
        """
        Return text for the ENV section of the final build stage.
        """
        env_section = ""
        self.log.debug("env_section = %s", env_section)
        return env_section

    @memoize
    def get_final_stage_text(self) -> str:
        """
        Return the text for the final stage, built up of the individual sections, in order.
        """
        final_stage_text = "\n".join(
            line
            for line in [
                f"\n# {self}",
                self.get_from_section(),
                self.get_copy_section(),
                self.get_env_section(),
                self.__get_workdir_section(),
                self.get_run_section(),
                'ENTRYPOINT [ "/bin/bash" ]',
            ]
            if line
        )

        self.log.debug("final_stage_text = %s", final_stage_text)

        return final_stage_text

    @memoize
    def get_from_section(self) -> str:
        """
        Return text for the FROM line of the final build stage.
        """
        from_section = f"FROM base AS {self.get_name()}"

        self.log.debug("from_section = %s", from_section)

        return from_section

    @memoize
    def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        """
        Return dockerfiles that describe build stages that come directly before this one.
        """
        input_dockerfiles = tuple()

        self.log.debug("input_dockerfiles = %s", input_dockerfiles)

        return input_dockerfiles

    @memoize
    def get_name(self) -> str:
        """
        Return the name of the final build stage, for e.g. `FROM <image> AS <name>`.
        """
        name = (
            f'{self.options.target.replace("/", "__")}__{self.cfg.section_name}'.lower()
        )

        self.log.debug("name = %s", name)

        return name

    @memoize
    def get_run_section(self) -> str:
        """
        Return text for the RUN line of the final build stage.
        """
        run_section = ""

        self.log.debug("run_section = %s", run_section)

        return run_section

    def __get_dockerfile_sections_recursive(
        self, dockerfile: GenAbcDockerfile, seen_dockerfiles
    ) -> Iterable[str]:
        text_parts = []
        if dockerfile not in seen_dockerfiles:
            seen_dockerfiles.add(dockerfile)
            for input_dockerfile in dockerfile.get_input_dockerfiles():
                text_parts += self.__get_dockerfile_sections_recursive(
                    input_dockerfile, seen_dockerfiles
                )
            if dockerfile.get_run_section() or dockerfile is self:
                text_parts.append(dockerfile.get_final_stage_text())
        return text_parts

    @memoize
    def get_text(self) -> str:
        """
        Return the full text for this dockerfile, including all build stages.
        """
        seen_dockerfiles = set()

        # Calculating which build stages to include in the full Dockerfile text has the same tricky
        # problem as mentioned in `get_copy_section`. We cannot have any build stages without a RUN
        # command or risk having Docker fail to build an image with inline caching enabled. If a
        # build stage exists that has no RUN section, we do not include it in the Dockerfile. The
        # only part of the dockerfile that is affected by this is the copy section for each stage,
        # as it will need to copy from any of the missing, empty stage's non-empty input stages
        # instead.

        text = "\n".join(
            [
                self.get_base_stages_text(),
                *self.__get_dockerfile_sections_recursive(self, seen_dockerfiles),
            ]
        )

        self.log.debug("text = %s", text)

        return text

    @memoize
    def __get_workdir_section(self) -> str:
        """
        Return text for the WORKDIR line of the final build stage.
        """
        workdir_section = f"WORKDIR {self.cfg.path.parent.image_build()}"

        self.log.debug("workdir_section = %s", workdir_section)

        return workdir_section

    @memoize
    def main(self) -> None:
        """
        Mainline interface.
        """
        self.log.info("Creating dockerfile %s", self.path)
        self.path.write_text(data=self.get_text())

    # pylint: disable=import-outside-toplevel
    #
    # Required to resolve cyclical dependency issues.
    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """

        # If we have any mixins, we may need to do some extra work in the
        # dockerfile to make sure that the mixins can work properly.
        if not self.options.mixins:
            dockerfile = self
        else:
            from gdev.cmd.gen._custom.dockerfile import GenCustomDockerfile

            dockerfile = GenCustomDockerfile(options=self.options, base_dockerfile=self)

        dockerfile.run()
        print(dockerfile.get_text())

    # pylint: enable=import-outside-toplevel
