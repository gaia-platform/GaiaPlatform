from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenRunDockerfile
from .._abc.image import GenAbcImage


class GenRunImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenRunDockerfile:
        return GenRunDockerfile(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcImage]:
        from ..apt.image import GenAptImage
        from ..gaia.image import GenGaiaImage
        from ..git.image import GenGitImage
        from ..pip.image import GenPipImage
        from ..web.image import GenWebImage

        direct_inputs = []
        for direct_input in [
            GenAptImage(self.options),
            GenGaiaImage(self.options),
            GenGitImage(self.options),
            GenPipImage(self.options),
            GenWebImage(self.options),
        ]:
            if await direct_input.cfg.get_lines():
                direct_inputs.append(direct_input)

        direct_inputs = tuple(direct_inputs)

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
