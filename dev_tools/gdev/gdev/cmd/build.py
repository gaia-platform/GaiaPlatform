from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.build import GenRunBuild


class Build(Dependency):

    @memoize
    async def cli_entrypoint(self) -> None:
        await GenRunBuild(self.options).cli_entrypoint()
