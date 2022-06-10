#!/usr/bin/env python3

###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

"""
Module to provide a base class for use by all the subcommand modules.
"""

# PYTHON_ARGCOMPLETE_OK

from __future__ import annotations
from abc import ABC, abstractmethod
from dataclasses import dataclass
from inspect import isabstract
import logging
import sys
from gdev.options import Options
from gdev.third_party.atools import memoize


@dataclass(frozen=True)
class GdevAction(ABC):
    """
    Class to provide a base class for use by all the subcommand modules.
    """

    options: Options

    def __hash__(self) -> int:
        return hash((type(self), self.options))

    def __repr__(self) -> str:
        return f"{type(self).__name__}({self.options.target})"

    def __str__(self) -> str:
        return repr(self)

    @property
    @memoize
    def log(self) -> logging.Logger:
        """
        Set up a logging instance for the current module.

        Note that the @memoize decorator means that this will only be setup
        once for any given instance of a class.
        """

        log = logging.getLogger(f"{self.__module__} ({self.options.target})")
        log.setLevel(self.options.log_level)
        log.propagate = False

        handler = logging.StreamHandler(sys.stderr)
        handler.setLevel(self.options.log_level)
        if handler.level > logging.DEBUG:
            formatter = logging.Formatter(f"({self.options.target}) %(message)s")
        else:
            formatter = logging.Formatter("%(levelname)s:%(name)s %(message)s")
        handler.setFormatter(formatter)
        log.addHandler(handler)
        return log

    @abstractmethod
    def main(self) -> None:
        """
        Main action to invoke for this class.
        """
        raise NotImplementedError

    @memoize
    def run(self) -> None:
        """
        Run the main action that is the focus of this class.
        """

        assert not isabstract(self), "This should never be abstract."
        assert hasattr(self, "main"), "Every action must have a main function."

        self.log.debug("Starting %s.main", str(type(self).__name__))
        self.main()
        self.log.debug("Finished %s.main", str(type(self).__name__))
