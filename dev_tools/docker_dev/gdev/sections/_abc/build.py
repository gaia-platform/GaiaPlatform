#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to help build a docker image from its component pieces.
"""

from __future__ import annotations
from abc import ABC, abstractmethod
import os
from typing import Iterable, Mapping

from gdev.custom.gaia_path import GaiaPath
from gdev.dependency import Dependency
from gdev.host import Host
from gdev.third_party.atools import memoize
from gdev.sections._abc.cfg import GenAbcCfg
from gdev.sections._abc.dockerfile import GenAbcDockerfile


# We require buildkit support for inline caching of multi-stage dockerfiles. It's also way faster
# and the terminal output is relatively sane.
os.environ["DOCKER_BUILDKIT"] = "1"
os.environ["DOCKER_CLI_EXPERIMENTAL"] = "enabled"


class GenAbcBuild(Dependency, ABC):
    """
    Build a Docker image from the rules in the `gdev.cfg` file in the current working directory.
    """

    @property
    def cfg(self) -> GenAbcCfg:
        """
        Get the configuration associated with a specific portion of the dockerfile.
        """
        return self.dockerfile.cfg

    @property
    @abstractmethod
    def dockerfile(self) -> GenAbcDockerfile:
        """
        Return the class that will be used to generate a specific portion of the dockerfile.
        """
        raise NotImplementedError

    @memoize
    def _get_actual_label_value(self, name: str) -> str:
        """
        Request that docker provide information about the label value for the current image.
        """
        if (
            line := Host.execute_and_get_line_sync(
                f"docker image inspect"
                f' --format="{{{{.Config.Labels.{name}}}}}"'
                f" {self.get_tag()}"
            )
        ) == '"<no value>"':
            value = ""
        else:
            value = line.strip('"')

        return value

    @memoize
    def _get_actual_label_value_by_name(self) -> Mapping[str, str]:
        """
        Get the hash of an image with the actual label values that are called
        for by the configuration.
        """
        return {"GitHash": self._get_actual_label_value(name="GitHash")}

    @memoize
    def get_actual_label_value_by_name(self) -> Mapping[str, str]:
        """
        Get the hash of an image with the actual label values that are called
        for by the configuration.
        """

        actual_label_value_by_name = self._get_actual_label_value_by_name()
        self.log.debug("actual_label_value_by_name = %s", actual_label_value_by_name)
        return actual_label_value_by_name

    @memoize
    def __get_base_build_names(self) -> Iterable[str]:

        seen_dockerfiles = set()

        def inner(dockerfile: GenAbcDockerfile) -> Iterable[str]:
            build_names = []
            if dockerfile not in seen_dockerfiles:
                seen_dockerfiles.add(dockerfile)
                for input_dockerfile in dockerfile.get_input_dockerfiles():
                    build_names += inner(input_dockerfile)
                if dockerfile.get_run_section() or dockerfile is self.dockerfile:
                    build_names.append(dockerfile.get_name())
            return build_names

        base_build_names = tuple(inner(self.dockerfile))

        self.log.debug("base_build_names = %s", base_build_names)

        return base_build_names

    @memoize
    def get_sha(self) -> str:
        """
        Determine the SHA hash associated with the current image.
        """

        if lines := Host.execute_and_get_lines_sync(
            f"docker image ls -q --no-trunc {self.get_tag()}"
        ):
            sha = next(iter(lines))
        else:
            sha = ""

        self.log.debug("sha = %s", sha)

        return sha

    @memoize
    def get_tag(self) -> str:
        """
        Construct a tag from the name of the dockerfile.
        """

        tag = f"{self.dockerfile.get_name()}:latest"

        self.log.debug("tag = %s", tag)

        return tag

    @memoize
    def __get_wanted_git_hash(self) -> str:
        """
        Request that GIT provides information about the SHA of the HEAD node.
        """
        wanted_git_hash = Host.execute_and_get_line_sync("git rev-parse HEAD")

        self.log.debug("wanted_git_hash = %s", wanted_git_hash)

        return wanted_git_hash

    def _get_wanted_label_value_by_name(self) -> Mapping[str, str]:
        """
        Get the hash of the image with the label values that are called for by the configuration.
        """
        return {"GitHash": self.__get_wanted_git_hash()}

    @memoize
    def get_wanted_label_value_by_name(self) -> Mapping[str, str]:
        """
        Get the hash of the image with the label values that are called for by the configuration.
        """
        wanted_label_value_by_name = self._get_wanted_label_value_by_name()

        self.log.debug("wanted_label_value_by_name = %s", wanted_label_value_by_name)

        return wanted_label_value_by_name

    @memoize
    def main(self) -> None:
        """
        Main action for this subcommand.
        """
        self.dockerfile.run()

        cached_images = ""
        if self.options.registry:
            cached_images = ",".join(
                [
                    f"{self.options.registry}/{base_build_name}:latest"
                    for base_build_name in self.__get_base_build_names()
                ]
            )
            cached_images = f"--cache-from {cached_images}"

        self.log.info('Creating image "%s"', self.get_tag())
        Host.execute_sync(
            f"docker buildx build"
            f" -f {self.dockerfile.path}"
            f" -t {self.get_tag()}"
            f"""{''.join([
                f' --label {name}="{value}"'
                for name, value
                in (self.get_wanted_label_value_by_name()).items()
            ])}"""
            # Keep metadata about layers so that they can be used as a cache source.
            f" --build-arg BUILDKIT_INLINE_CACHE=1"
            f" --platform linux/{self.options.platform}"
            # Required to run production.
            f" --shm-size 1gb"
            # Allow cloning repos with ssh.
            f" --ssh default"
            f" {cached_images}"
            f" {GaiaPath.repo()}"
        )
        Host.execute_sync("docker image prune -f")
