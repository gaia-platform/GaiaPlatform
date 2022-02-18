"""
Module to provide handling for the 'dockerfile' subcommand.
"""
from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.dockerfile import GenRunDockerfile


class Dockerfile(Dependency):
    """
    Class to encapsulate the 'cfg' subcommand.
    """

    @memoize
    async def cli_entrypoint(self) -> None:
        """
        Entry point for the command line.
        """
        await GenRunDockerfile(self.options).cli_entrypoint()
