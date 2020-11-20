from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen.run.dockerfile import GenRunDockerfile


class Dockerfile(Dependency):

    @memoize
    async def cli_entrypoint(self) -> None:
        await GenRunDockerfile(self.options).cli_entrypoint()
