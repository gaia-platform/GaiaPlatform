from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenGaiaCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenGaiaDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenGaiaCfg:
        return GenGaiaCfg(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcDockerfile]:
        from ..dockerfile import GenDockerfile

        direct_inputs = []

        for line in await self.cfg.get_lines():
            direct_inputs.append(GenDockerfile(replace(self.options, target=line)))

        direct_inputs = tuple(direct_inputs)

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
