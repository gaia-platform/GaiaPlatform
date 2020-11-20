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
    async def get_copy_section(self) -> str:
        copy_section_parts = []
        if copy_section := await super()._get_copy_section():
            copy_section_parts.append(copy_section)
        if set(self.cfg.path.parent.iterdir()) - {self.cfg.path}:
            copy_section_parts.append(
                f'COPY {self.cfg.path.parent.context()} {self.cfg.path.parent.image_source()}'
            )
        copy_section = '\n'.join(copy_section_parts)

        self.log.debug(f'{copy_section = }')

        return copy_section

    @memoize
    async def get_input_dockerfiles(self) -> Iterable[GenAbcDockerfile]:
        from ..output.dockerfile import GenOutputDockerfile

        input_dockerfiles = []
        for section_line in await self.cfg.get_section_lines():
            input_dockerfiles.append(
                GenOutputDockerfile(replace(self.options, target=section_line))
            )
        input_dockerfiles = tuple(input_dockerfiles)

        self.log.debug(f'{input_dockerfiles = }')

        return input_dockerfiles
