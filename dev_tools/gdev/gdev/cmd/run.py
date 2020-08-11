from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen._custom.run import GenCustomRun
from .gen.run.run import GenRunRun


class Run(Dependency):

    @memoize
    async def cli_entrypoint(self) -> None:
        if self.options.mixins:
            await GenCustomRun(self.options).cli_entrypoint()
        else:
            await GenRunRun(self.options).cli_entrypoint()
