"""
Module to provide for the `cfg` subcommand entry point.
"""
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.build import GenRunBuild


class Build(Dependency):
    """
    Class to provide for the `build` subcommand entry point.
    """

    @memoize
    def cli_entrypoint(self) -> None:
        """
        Execution entrypoint for this module.
        """
        GenRunBuild(self.options).cli_entrypoint()
