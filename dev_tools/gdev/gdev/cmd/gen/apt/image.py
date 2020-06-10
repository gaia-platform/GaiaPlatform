from typing import Iterable

from gdev.third_party.atools import memoize
from .dockerfile import GenAptDockerfile
from .._abc.image import GenAbcImage


class GenAptImage(GenAbcImage):

    @property
    def dockerfile(self) -> GenAptDockerfile:
        return GenAptDockerfile(self.options)

    @memoize
    async def get_children(self) -> Iterable[GenAbcImage]:

        children = tuple()

        self.log.debug(f'{children = }')

        return children
