from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenDockerfile
from ._abc.image import GenAbcImage


class GenImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenDockerfile:
        return GenDockerfile(self.options)

    @memoize
    async def get_direct_inputs(self) -> Iterable[GenAbcImage]:
        from .run.image import GenRunImage

        direct_inputs = tuple([GenRunImage(self.options)])

        self.log.debug(f'{direct_inputs = }')

        return direct_inputs
