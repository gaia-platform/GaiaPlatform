from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenGaiaDockerfile
from .._abc.image import GenAbcImage


class GenGaiaImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenGaiaDockerfile:
        return GenGaiaDockerfile(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcImage]:
        from ..image import GenImage

        direct_inputs = []

        for line in await self.cfg.get_lines():
            direct_inputs.append(GenImage(replace(self.options, target=line)))

        direct_inputs = tuple(direct_inputs)

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
