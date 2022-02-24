#!/usr/bin/env python3

#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

"""
Module to provide a base class for use by all the subcommand modules.
"""

# PYTHON_ARGCOMPLETE_OK

from __future__ import annotations
from dataclasses import dataclass
from inspect import isabstract
import logging
import sys
from gdev.options import Options
from gdev.third_party.atools import memoize


@dataclass(frozen=True)
class Dependency:
    """
    Class to provide a base class for use by all the subcommand modules.
    """

    options: Options

    # These two classes are only present to handle the Abort exception,
    # and early exit in some cases.
    class Exception(Exception):
        """
        Useless exception to be refactored out.
        """

    class Abort(Exception):
        """
        Exception to know we aborted.
        """

        def __str__(self) -> str:
            return f"Abort: {super().__str__()}"

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

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        self.run()

    @memoize
    def run(self) -> None:
        """
        Run the main action that is the focus of this class.
        """

        if isabstract(self):
            return

        if hasattr(self, "main"):
            self.log.debug("Starting %s.main", str(type(self).__name__))
            self.main()
            self.log.debug("Finished %s.main", str(type(self).__name__))
