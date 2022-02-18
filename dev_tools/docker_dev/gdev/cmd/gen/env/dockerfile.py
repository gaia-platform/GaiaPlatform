from dataclasses import replace
from typing import Iterable

from gdev.third_party.atools import memoize
from .cfg import GenEnvCfg
from .._abc.dockerfile import GenAbcDockerfile


class GenEnvDockerfile(GenAbcDockerfile):

    @property
    def cfg(self) -> GenEnvCfg:
        return GenEnvCfg(self.options)

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..gaia.dockerfile import GenGaiaDockerfile

        input_dockerfiles = []
        for section_line in await GenGaiaDockerfile(self.options).cfg.get_section_lines():
            input_dockerfiles.append(GenEnvDockerfile(replace(self.options, target=section_line)))
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug(f'{input_dockerfiles}')

        return input_dockerfiles
