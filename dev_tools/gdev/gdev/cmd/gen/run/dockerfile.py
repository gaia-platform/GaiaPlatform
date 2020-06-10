from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenRunCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenRunDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenRunCfg:
        return GenRunCfg(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcDockerfile]:
        from ..apt.dockerfile import GenAptDockerfile
        from ..gaia.dockerfile import GenGaiaDockerfile
        from ..git.dockerfile import GenGitDockerfile
        from ..pip.dockerfile import GenPipDockerfile
        from ..web.dockerfile import GenWebDockerfile

        direct_inputs = []
        for direct_input in [
            GenAptDockerfile(self.options),
            GenGaiaDockerfile(self.options),
            GenGitDockerfile(self.options),
            GenPipDockerfile(self.options),
            GenWebDockerfile(self.options),
        ]:
            if await direct_input.cfg.get_lines():
                direct_inputs.append(direct_input)

        direct_inputs = tuple(direct_inputs)

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
