from typing import Iterable

from gdev.custom.pathlib import Path
from gdev.third_party.atools import memoize
from .._abc.cfg import GenAbcCfg


class GenCustomCfg(GenAbcCfg):

    @memoize
    async def get_lines(self) -> Iterable[str]:
        lines = tuple([f'{Path.mixin().context() / mixin}' for mixin in self.options.mixins])

        self.log.info(f'{lines = }')

        return lines
