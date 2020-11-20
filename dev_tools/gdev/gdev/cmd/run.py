from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.run import GenRunRun


class Run(Dependency):

    @memoize
    async def cli_entrypoint(self) -> None:
        await GenRunRun(self.options).cli_entrypoint()
