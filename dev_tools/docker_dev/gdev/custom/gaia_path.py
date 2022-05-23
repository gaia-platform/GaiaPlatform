#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide various pathing utilities to make things simpler.
"""

from __future__ import annotations

from logging import getLogger
from pathlib import PosixPath
from subprocess import check_output
from textwrap import indent
from typing import Union


log = getLogger(__name__)


class GaiaPath(PosixPath):
    """
    Class to provide various pathing utilities to make things simpler.
    """

    _repo: GaiaPath = None

    def context(self) -> GaiaPath:
        """
        Determine the basis on which to generated paths.
        """
        if self.is_absolute():
            return self.relative_to(self.repo())
        return self

    def image_build(self) -> GaiaPath:
        """
        Generate a path to the build directory.
        """
        return GaiaPath.build(self.context())

    def image_source(self) -> GaiaPath:
        """
        Generate a path to the source directory.
        """
        return GaiaPath.source(self.context())

    @classmethod
    def mixin(cls) -> GaiaPath:
        """
        Determine the path to where the mixins are stored.
        """
        return cls.repo() / "dev_tools" / "gdev" / "mixin"

    @classmethod
    def repo(cls) -> GaiaPath:
        """
        Determine the path to root of the repository.
        """
        if cls._repo is None:
            repo = GaiaPath(
                check_output(
                    "git rev-parse --show-toplevel".split(),
                    cwd=f"{GaiaPath(__file__).parent}",
                )
                .decode()
                .strip()
            )

            log.debug("repo = %s", repo)

            cls._repo = repo

        return cls._repo

    @classmethod
    def build(cls, path: Union[GaiaPath, str] = "/") -> GaiaPath:
        """
        Compute a path along the "/build" tree in the container.
        """
        return GaiaPath("/build") / path

    @classmethod
    def source(cls, path: Union[GaiaPath, str] = "/") -> GaiaPath:
        """
        Compute a path along the "/source" tree in the container.
        """
        return GaiaPath("/source") / path

    def write_text(self, data: str, encoding=None, errors=None) -> None:
        """
        Write the specified text to the given file.
        """
        log.debug("Writing to %s, text: \n%s", self, indent(data, "    "))
        self.parent.mkdir(parents=True, exist_ok=True)
        super().write_text(data, encoding=encoding, errors=errors)
