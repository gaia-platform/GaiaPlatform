"""
Module to provide for the `run` subcommand entry point.
"""

from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.run import GenRunRun


class Run(Dependency):
    """
    Class to provide for the `run` subcommand entry point.
    """

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunRun(self.options).cli_entrypoint()
