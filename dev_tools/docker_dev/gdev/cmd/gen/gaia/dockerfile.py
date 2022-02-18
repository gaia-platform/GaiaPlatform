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
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..run.dockerfile import GenRunDockerfile

        input_dockerfiles = []
        for section_line in await self.cfg.get_section_lines():
            input_dockerfiles.append(GenRunDockerfile(replace(self.options, target=section_line)))
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles
