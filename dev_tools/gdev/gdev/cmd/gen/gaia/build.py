from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenGaiaDockerfile
from .._abc.build import GenAbcBuild


class GenGaiaBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenGaiaDockerfile:
        return GenGaiaDockerfile(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcBuild]:
        from ..apt.build import GenAptBuild
        from ..git.build import GenGitBuild
        from ..pip.build import GenPipBuild
        from ..run.build import GenRunBuild
        from ..web.build import GenWebBuild

        direct_inputs = []
        for line in await self.cfg.get_lines():
            options = replace(self.options, target=line)
            if await (run_build := GenRunBuild(options)).cfg.get_lines():
                direct_inputs.append(run_build)
            else:
                for build in [
                        GenAptBuild(options),
                        GenGaiaBuild(options),
                        GenGitBuild(options),
                        GenPipBuild(options),
                        GenWebBuild(options),
                ]:
                    if await build.cfg.get_lines():
                        direct_inputs.append(build)
        direct_inputs = tuple(direct_inputs)

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
