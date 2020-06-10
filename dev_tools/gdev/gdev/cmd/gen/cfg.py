from typing import Iterable

from gdev.third_party.atools import memoize
from ._abc.cfg import GenAbcCfg
from .run.cfg import GenRunCfg


class GenCfg(GenAbcCfg):

    @memoize
    async def get_section_name(self) -> str:
        section_name = ''

        self.log.debug(f'{section_name = }')

        return section_name

    @memoize
    async def get_lines(self) -> Iterable[str]:
        lines = await GenRunCfg(self.options).get_lines()

        self.log.debug(f'{lines = }')

        return lines
