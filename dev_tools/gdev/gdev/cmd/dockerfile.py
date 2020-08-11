from gdev.dependency import Dependency
from gdev.third_party.atools import memoize
from .gen._custom.dockerfile import GenCustomDockerfile
from .gen.run.dockerfile import GenRunDockerfile


class Dockerfile(Dependency):

    @memoize
    async def cli_entrypoint(self) -> None:
        if self.options.mixins:
            await GenCustomDockerfile(self.options).cli_entrypoint()
        else:
            await GenRunDockerfile(self.options).cli_entrypoint()
