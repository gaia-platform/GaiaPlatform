from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.push import GenRunPush


class Push(Dependency):

    @memoize
    async def cli_entrypoint(self) -> None:
        await GenRunPush(self.options).cli_entrypoint()
