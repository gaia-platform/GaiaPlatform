#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to parse the target gdev.cfg for build rules.
"""

from abc import ABC
from inspect import getfile
import re
from typing import FrozenSet, Iterable, Pattern

from gdev.custom.gaia_path import GaiaPath
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize


class GenAbcCfg(Dependency, ABC):
    """
    Class to parse the target gdev.cfg for build rules.
    """

    @property
    @memoize
    def path(self) -> GaiaPath:
        """
        Determine the path to the configuration file that we are observing.
        """
        path = GaiaPath.repo() / self.options.target / "gdev.cfg"
        self.log.debug("path = %s", path)
        return path

    @property
    def section_name(self) -> str:
        """
        Determine the section name in the configuration based on the type of the class.
        """
        return GaiaPath(getfile(type(self))).parent.name.strip("_")

    @memoize
    def __get_begin_pattern(self) -> Pattern:
        """
        Get the regex pattern that identifies the beginning of the section.
        """

        begin_pattern = re.compile(fr"^\[{self.section_name}]$")
        self.log.debug("begin_pattern = %s", begin_pattern)
        return begin_pattern

    @memoize
    def __get_end_pattern(self) -> Pattern:
        """
        Get the regex pattern that identifies the end of the section.
        """
        end_pattern = re.compile(r"^(# .*|)\[.+]$")
        self.log.debug("end_pattern = %s", end_pattern)
        return end_pattern

    # pylint: disable=eval-used
    @staticmethod
    @memoize
    def get_lines(cfg_enables: FrozenSet[str], path: GaiaPath) -> Iterable[str]:
        """
        Get the various lines for the section with the inline notations like
        `{enable_if('CI_GitHub')}` replaced with `# enable by setting "{'CI_GitHub'}": `.
        """
        # Per suggestions from https://realpython.com/python-eval-function/:
        # - `__locals` field set to an empty dictionary
        # - `__globals` field set to contain empty `__builtins__` item

        return tuple(
            (
                eval(
                    f'fr""" {line} """',
                    {
                        "build_dir": GaiaPath.build,
                        "enable_if": lambda enable: ""
                        if enable in cfg_enables
                        else f'# enable by setting "{enable}": ',
                        "enable_if_not": lambda enable: ""
                        if enable not in cfg_enables
                        else f'# enable by not setting "{enable}": ',
                        "enable_if_any": lambda *enables: ""
                        if set(enables) & cfg_enables
                        else f'# enable by setting any of "{sorted(set(enables))}": ',
                        "enable_if_not_any": lambda *enables: ""
                        if not (set(enables) & cfg_enables)
                        else f'# enable by not setting any of "{sorted(set(enables))}": ',
                        "enable_if_all": lambda *enables: ""
                        if set(enables) in cfg_enables
                        else f'# enable by setting all of "{sorted(set(enables))}": ',
                        "enable_if_not_all": lambda *enables: ""
                        if not (set(enables) in cfg_enables)
                        else f'# enable by not setting all of "{sorted(set(enables))}": ',
                        "source_dir": GaiaPath.source,
                        "__builtins__": {},
                    },
                    {},
                )[1:-1]
                for line in (GenAbcCfg.__get_raw_text(path)).splitlines()
            )
        )

    # pylint: enable=eval-used

    @staticmethod
    @memoize
    def __get_raw_text(path: GaiaPath) -> str:
        if not path.is_file():
            raise Dependency.Abort(f'File "<repo_root>/{path.context()}" must exist.')

        text = path.read_text()
        return text

    @memoize
    def get_section_lines(self) -> Iterable[str]:
        """
        Get all the lines for the current section.
        """

        lines = self.get_lines(cfg_enables=self.options.cfg_enables, path=self.path)

        ilines = iter(lines)
        section_lines = []
        begin_pattern = self.__get_begin_pattern()
        for iline in ilines:
            if begin_pattern.match(iline):
                break

        end_pattern = self.__get_end_pattern()
        for iline in ilines:
            if end_pattern.match(iline):
                break
            section_lines.append(iline)

        ilines = iter(section_lines)
        section_lines = []
        for iline in ilines:
            line_parts = [iline]
            while line_parts[-1].endswith("\\"):
                if not (next_line := next(ilines)).strip().startswith("#"):
                    line_parts.append(next_line)
            section_lines.append("\n    ".join(line_parts))

        section_lines = [
            section_line
            for section_line in section_lines
            if (section_line and not section_line.startswith("#"))
        ]

        section_lines = tuple(section_lines)

        self.log.debug("section_lines = %s", section_lines)

        return section_lines

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        print(f"[{self.section_name}]\n" + "\n".join(self.get_section_lines()))
