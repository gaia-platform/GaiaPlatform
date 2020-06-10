from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenPreRunDockerfile
from .._abc.build import GenAbcBuild


class GenPreRunBuild(GenAbcBuild):

    @property
    def dockerfile(self) -> GenPreRunDockerfile:
        return GenPreRunDockerfile(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcBuild]:
        from ..apt.build import GenAptBuild
        from ..gaia.build import GenGaiaBuild
        from ..git.build import GenGitBuild
        from ..pip.build import GenPipBuild
        from ..web.build import GenWebBuild

        direct_inputs = []
        for direct_input in [
            GenAptBuild(self.options),
            GenGaiaBuild(self.options),
            GenGitBuild(self.options),
            GenPipBuild(self.options),
            GenWebBuild(self.options),
        ]:
            if await direct_input.cfg.get_lines():
                direct_inputs.append(direct_input)

        direct_inputs = tuple(direct_inputs)

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
